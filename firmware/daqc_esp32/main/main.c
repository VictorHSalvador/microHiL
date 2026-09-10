#include <string.h>

#include "daqc_profile.h"
#include "daqc_protocol.h"
#include "daqc_transport.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static daqc_parser_t g_parser;
static volatile uint8_t g_state = DAQC_COMMAND_DISABLE;
static volatile uint16_t g_read_ack;
static volatile int64_t g_read_ack_time_us;
static volatile uint16_t g_sequence;

static bool SendConfigState(uint8_t command) {
    uint8_t frame[5];
    const size_t size = DaqcEncodeConfig(frame, sizeof(frame), command, true, g_state);
    return size && uart_write_bytes(UART_NUM_0, (const char *)frame, size) == (int)size;
}

static bool ProcessFrame(const daqc_frame_t *frame, void *context) {
    (void)context;
    if (frame->mid == DAQC_MID_CONFIG) {
        if (frame->command == DAQC_COMMAND_DISABLE || frame->command == DAQC_COMMAND_ENABLE || frame->command == DAQC_COMMAND_STREAMING) {
            if (frame->command == DAQC_COMMAND_STREAMING && !DaqcProfileAcquire((uint8_t[28]){0})) return SendConfigState(frame->command);
            g_state = frame->command;
            if (g_state != DAQC_COMMAND_STREAMING) DaqcProfileSetSafeOutputs();
        }
        return SendConfigState(frame->command);
    }
    if (frame->mid == DAQC_MID_DATA && g_state == DAQC_COMMAND_STREAMING) return DaqcProfileApplyActuation(frame->payload);
    if (frame->mid == DAQC_MID_READ_ACK && g_state == DAQC_COMMAND_STREAMING) { g_read_ack = frame->sequence; g_read_ack_time_us = esp_timer_get_time(); return true; }
    if (frame->mid == DAQC_MID_XRCE) return DaqcTransportAcceptXrce(frame->payload, frame->payload_size);
    return true;
}

static void CommunicationTask(void *argument) {
    (void)argument;
    uint8_t bytes[128];
    while (true) { const int count = uart_read_bytes(UART_NUM_0, bytes, sizeof(bytes), pdMS_TO_TICKS(5)); if (count > 0) (void)DaqcParserFeed(&g_parser, bytes, (size_t)count, ProcessFrame, NULL); }
}

static void IoTask(void *argument) {
    (void)argument;
    uint8_t payload[28];
    while (true) {
        if (g_state == DAQC_COMMAND_STREAMING && DaqcProfileAcquire(payload)) {
            uint8_t frame[33];
            const size_t size = DaqcEncodeData(frame, sizeof(frame), g_sequence++, payload, sizeof(payload));
            if (size) (void)uart_write_bytes(UART_NUM_0, (const char *)frame, size);
        }
        if (g_state == DAQC_COMMAND_STREAMING && esp_timer_get_time() - g_read_ack_time_us >= 60000000LL) { g_state = DAQC_COMMAND_DISABLE; DaqcProfileSetSafeOutputs(); }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_main(void) {
    DaqcParserInit(&g_parser);
    if (!DaqcTransportInit()) return;
    g_read_ack_time_us = esp_timer_get_time();
    xTaskCreatePinnedToCore(CommunicationTask, "daqc_comm", 6144U, NULL, 8U, NULL, 0);
    xTaskCreatePinnedToCore(IoTask, "daqc_io", 4096U, NULL, 9U, NULL, 1);
}
