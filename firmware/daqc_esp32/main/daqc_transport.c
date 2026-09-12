/* Implementação do módulo daqc transport. */
#include "daqc_transport.h"

#include "daqc_protocol.h"
#include "driver/uart.h"

#define DAQC_UART UART_NUM_0
#define DAQC_BAUD_RATE 152000
#define DAQC_UART_BUFFER_SIZE 512
#define DAQC_XRCE_STREAM_SIZE DAQC_XRCE_MTU

static SemaphoreHandle_t g_tx_mutex;
static StreamBufferHandle_t g_xrce_stream;

bool DaqcTransportInit(void) {
    const uart_config_t config = {.baud_rate = DAQC_BAUD_RATE, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
                                  .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT};
    if (uart_param_config(DAQC_UART, &config) != ESP_OK || uart_set_pin(DAQC_UART, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK ||
        uart_driver_install(DAQC_UART, DAQC_UART_BUFFER_SIZE, DAQC_UART_BUFFER_SIZE, 0, NULL, 0) != ESP_OK) return false;
    g_tx_mutex = xSemaphoreCreateMutex();
    g_xrce_stream = xStreamBufferCreate(DAQC_XRCE_STREAM_SIZE, 1U);
    return g_tx_mutex && g_xrce_stream;
}

int DaqcTransportReadRaw(uint8_t *buffer, size_t capacity, uint32_t timeout_ms) {
    if (!buffer || capacity == 0U) return -1;
    return uart_read_bytes(DAQC_UART, buffer, capacity, pdMS_TO_TICKS(timeout_ms));
}

static bool SendFrame(const uint8_t *frame, size_t frame_size) {
    if (!frame_size || !g_tx_mutex || xSemaphoreTake(g_tx_mutex, pdMS_TO_TICKS(5)) != pdTRUE) return false;
    const int written = uart_write_bytes(DAQC_UART, (const char *)frame, frame_size);
    (void)xSemaphoreGive(g_tx_mutex);
    return written == (int)frame_size;
}

bool DaqcTransportSendConfig(uint8_t command, uint8_t status) {
    uint8_t frame[5];
    const size_t frame_size = DaqcEncodeConfig(frame, sizeof(frame), command, true, status);
    return SendFrame(frame, frame_size);
}

bool DaqcTransportSendAcquisition(uint16_t sequence, const uint8_t payload[DAQC_ACQUISITION_SIZE]) {
    uint8_t frame[5U + DAQC_ACQUISITION_SIZE];
    const size_t frame_size = DaqcEncodeData(frame, sizeof(frame), sequence, payload, DAQC_ACQUISITION_SIZE);
    return SendFrame(frame, frame_size);
}

static bool DaqcTransportSendXrce(const uint8_t *payload, size_t payload_size) {
    uint8_t frame[5U + DAQC_XRCE_MTU];
    const size_t frame_size = DaqcEncodeXrce(frame, sizeof(frame), payload, payload_size);
    return SendFrame(frame, frame_size);
}

bool DaqcTransportAcceptXrce(const uint8_t *payload, size_t payload_size) {
    return g_xrce_stream && payload && xStreamBufferSend(g_xrce_stream, payload, payload_size, 0U) == payload_size;
}

bool DaqcTransportOpen(struct uxrCustomTransport *transport) { return transport && g_xrce_stream; }
bool DaqcTransportClose(struct uxrCustomTransport *transport) { return transport != NULL; }
size_t DaqcTransportWrite(struct uxrCustomTransport *transport, const uint8_t *buffer, size_t length, uint8_t *error_code) {
    (void)transport;
    if (error_code) *error_code = 0U;
    return DaqcTransportSendXrce(buffer, length) ? length : 0U;
}
size_t DaqcTransportRead(struct uxrCustomTransport *transport, uint8_t *buffer, size_t length, int timeout_ms, uint8_t *error_code) {
    (void)transport;
    if (error_code) *error_code = 0U;
    if (!g_xrce_stream || !buffer || length == 0U) return 0U;
    const TickType_t timeout = timeout_ms <= 0 ? 0U : pdMS_TO_TICKS((uint32_t)timeout_ms);
    return xStreamBufferReceive(g_xrce_stream, buffer, length, timeout);
}
