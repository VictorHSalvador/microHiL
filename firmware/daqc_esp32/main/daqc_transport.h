#ifndef DAQC_TRANSPORT_H
#define DAQC_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "uxr/client/transport.h"

bool DaqcTransportInit(void);
bool DaqcTransportSend(uint8_t mid, const uint8_t *payload, size_t payload_size);
bool DaqcTransportAcceptXrce(const uint8_t *payload, size_t payload_size);
bool DaqcTransportOpen(struct uxrCustomTransport *transport);
bool DaqcTransportClose(struct uxrCustomTransport *transport);
size_t DaqcTransportWrite(struct uxrCustomTransport *transport, const uint8_t *buffer, size_t length, uint8_t *error_code);
size_t DaqcTransportRead(struct uxrCustomTransport *transport, uint8_t *buffer, size_t length, int timeout_ms, uint8_t *error_code);

#endif
