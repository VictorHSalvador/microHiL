#ifndef DAQC_TRANSPORT_H
#define DAQC_TRANSPORT_H

/* Contrato público do módulo daqc transport. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "uxr/client/transport.h"

#include "daqc_protocol.h"

bool DaqcTransportInit(void);
int DaqcTransportReadRaw(uint8_t *buffer, size_t capacity, uint32_t timeout_ms);
bool DaqcTransportSendConfig(uint8_t command, uint8_t status);
bool DaqcTransportSendAcquisition(uint16_t sequence, const uint8_t payload[DAQC_ACQUISITION_SIZE]);
bool DaqcTransportAcceptXrce(const uint8_t *payload, size_t payload_size);
bool DaqcTransportOpen(struct uxrCustomTransport *transport);
bool DaqcTransportClose(struct uxrCustomTransport *transport);
size_t DaqcTransportWrite(struct uxrCustomTransport *transport, const uint8_t *buffer, size_t length, uint8_t *error_code);
size_t DaqcTransportRead(struct uxrCustomTransport *transport, uint8_t *buffer, size_t length, int timeout_ms, uint8_t *error_code);

#endif
