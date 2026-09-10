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

static void CommunicationTask(void *argument) {
    (void)argument;
    uint8_t bytes[128];
    while (true) {
        const int count = DaqcTransportReadRaw(bytes, sizeof(bytes), 5);
        if (count > 0) (void)DaqcParserFeed(&g_parser, bytes, (size_t)count, ProcessFrame, NULL);
    }
}

static void IoTask(void *argument) {
    (void)argument;
    uint8_t payload[28];
    while (true) {
        if (DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING && DaqcProfileAcquire(payload)) {
            (void)DaqcTransportSendAcquisition(g_sequence++, payload);
        }
        if (DaqcControlState(&g_control) == DAQC_COMMAND_STREAMING && esp_timer_get_time() - g_read_ack_time_us >= 60000000LL) {
            if (DaqcControlApplyCommand(&g_control, DAQC_COMMAND_DISABLE) == DAQC_CONTROL_OK) DaqcRosRequestStatePublication();
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_main(void) {
    DaqcParserInit(&g_parser);
    if (!DaqcControlInit(&g_control) || !DaqcTransportInit()) return;
    g_read_ack_time_us = esp_timer_get_time();
    (void)DaqcRosStart(&g_control);
    xTaskCreatePinnedToCore(CommunicationTask, "daqc_comm", 6144U, NULL, 8U, NULL, 0);
    xTaskCreatePinnedToCore(IoTask, "daqc_io", 4096U, NULL, 9U, NULL, 1);
}
