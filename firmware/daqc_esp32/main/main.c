/* Implementação do módulo main. */
#include <string.h>

#include "daqc_control.h"
#include "daqc_profile.h"
#include "daqc_protocol.h"
#include "daqc_ros.h"
#include "daqc_transport.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static daqc_parser_t g_parser;
static daqc_control_t g_control;
static volatile uint16_t g_read_ack;
static volatile int64_t g_read_ack_time_us;
static volatile uint16_t g_sequence;
static volatile bool g_read_ack_seen;

#define DAQC_ROS_SUPERVISOR_TASK_PRIORITY 3U
#define DAQC_ROS_SUPERVISOR_TASK_STACK_SIZE 8192U
#define DAQC_ROS_RETRY_INTERVAL_MS 1000U
#define DAQC_COMMUNICATION_STARTUP_DELAY_MS 10U
#define DAQC_COMMUNICATION_TASK_STACK_SIZE 4096U
#define DAQC_COMMUNICATION_IDLE_WAIT_US 1000U
#define DAQC_COMMUNICATION_WAIT_GUARD_MS 10U

static TaskHandle_t g_communication_task;
static esp_timer_handle_t g_communication_wait_timer;
static volatile bool g_communication_timer_failure;

static bool SendConfigState(uint8_t command) {
    return DaqcTransportSendConfig(command, DaqcControlState(&g_control));
}

static bool IsNewerSequence(uint16_t sequence, uint16_t previous) {
    const uint16_t distance = (uint16_t)(sequence - previous);
    return distance != 0U && distance < 0x8000U;
}

static bool ProcessFrame(const daqc_frame_t *frame, void *context) {
    (void)context;
    if (frame->mid == DAQC_MID_CONFIG) {
        if (DaqcControlApplyCommand(&g_control, frame->command) == DAQC_CONTROL_OK) {
            if (DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING) {
                g_read_ack_seen = false;
                g_read_ack_time_us = esp_timer_get_time();
            }
            DaqcRosRequestStatePublication();
        }
        return SendConfigState(frame->command);
    }
    if (frame->mid == DAQC_MID_DATA && DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING) return DaqcProfileApplyActuation(frame->payload);
    if (frame->mid == DAQC_MID_READ_ACK && DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING) {
        if (!g_read_ack_seen || IsNewerSequence(frame->sequence, g_read_ack)) {
            g_read_ack = frame->sequence;
            g_read_ack_seen = true;
            g_read_ack_time_us = esp_timer_get_time();
        }
        return true;
    }
    if (frame->mid == DAQC_MID_XRCE) return DaqcTransportAcceptXrce(frame->payload, frame->payload_size);
    return true;
}

static void CommunicationWaitTimerCallback(void *argument) {
    (void)argument;
    if (g_communication_task) xTaskNotifyGive(g_communication_task);
}

static void CommunicationTask(void *argument) {
    (void)argument;
    uint8_t bytes[128];
    while (true) {
        const int count = DaqcTransportReadRaw(bytes, sizeof(bytes), 0U);
        if (count > 0) (void)DaqcParserFeed(&g_parser, bytes, (size_t)count, ProcessFrame, NULL);
        if (count <= 0) {
            (void)ulTaskNotifyTake(pdTRUE, 0U);
            const esp_err_t timer_status = esp_timer_start_once(g_communication_wait_timer, DAQC_COMMUNICATION_IDLE_WAIT_US);
            if (timer_status == ESP_OK) {
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(DAQC_COMMUNICATION_WAIT_GUARD_MS)) == 0U) {
                    g_communication_timer_failure = true;
                }
                (void)esp_timer_stop(g_communication_wait_timer);
            } else {
                /* A failed timer must yield instead of monopolizing core 0. */
                g_communication_timer_failure = true;
                vTaskDelay(1U);
            }
        }
    }
}

static void IoTask(void *argument) {
    (void)argument;
    uint8_t payload[28];
    int64_t next_acquisition_us = esp_timer_get_time();
    while (true) {
        if (DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING) {
            const int64_t now_us = esp_timer_get_time();
            if (now_us >= next_acquisition_us) {
                const uint32_t period_us = DaqcProfileAcquisitionPeriodUs();
                if (period_us > 0U && DaqcProfileAcquire(payload)) (void)DaqcTransportSendAcquisition(g_sequence++, payload);
                next_acquisition_us = now_us + period_us;
            }
        } else {
            next_acquisition_us = esp_timer_get_time();
        }
        if (DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING && esp_timer_get_time() - g_read_ack_time_us >= 60000000LL) {
            if (DaqcControlApplyCommand(&g_control, DAQC_COMMAND_DISABLE) == DAQC_CONTROL_OK) DaqcRosRequestStatePublication();
        }
        /* A one-tick delay prevents polling when the configured rate is 100 Hz. */
        vTaskDelay(1U);
    }
}

static void RosSupervisorTask(void *argument) {
    (void)argument;
    while (true) {
        if (DaqcControlState(&g_control) != DAQC_COMMAND_STREAMING) (void)DaqcRosStart(&g_control);
        vTaskDelay(pdMS_TO_TICKS(DAQC_ROS_RETRY_INTERVAL_MS));
    }
}

void app_main(void) {
    DaqcParserInit(&g_parser);
    if (!DaqcControlInit(&g_control) || !DaqcTransportInit()) return;
    const esp_timer_create_args_t wait_timer = {
        .callback = CommunicationWaitTimerCallback,
        .arg = NULL,
        .name = "daqc_comm_wait",
    };
    if (esp_timer_create(&wait_timer, &g_communication_wait_timer) != ESP_OK) return;
    g_read_ack_time_us = esp_timer_get_time();
    if (xTaskCreatePinnedToCore(CommunicationTask, "daqc_comm", DAQC_COMMUNICATION_TASK_STACK_SIZE, NULL, 8U,
                                &g_communication_task, 0) != pdPASS) return;
    vTaskDelay(pdMS_TO_TICKS(DAQC_COMMUNICATION_STARTUP_DELAY_MS));
    if (xTaskCreatePinnedToCore(RosSupervisorTask, "daqc_ros_supervisor", DAQC_ROS_SUPERVISOR_TASK_STACK_SIZE, NULL,
                                DAQC_ROS_SUPERVISOR_TASK_PRIORITY, NULL, 0) != pdPASS) return;
    if (xTaskCreatePinnedToCore(IoTask, "daqc_io", 4096U, NULL, 9U, NULL, 1) != pdPASS) return;
}
