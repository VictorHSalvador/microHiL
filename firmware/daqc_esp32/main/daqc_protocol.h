#ifndef DAQC_PROTOCOL_H
#define DAQC_PROTOCOL_H

/* Contrato público do módulo daqc protocol. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DAQC_SYNC_FIRST 0x59U
#define DAQC_SYNC_SECOND 0x72U
#define DAQC_MID_CONFIG 0x01U
#define DAQC_MID_DATA 0x02U
#define DAQC_MID_READ_ACK 0x03U
#define DAQC_MID_XRCE 0x04U
#define DAQC_COMMAND_DISABLE 0x01U
#define DAQC_COMMAND_ENABLE 0x02U
#define DAQC_COMMAND_STREAMING 0x03U
#define DAQC_ACQUISITION_SIZE 28U
#define DAQC_ACTUATION_SIZE 21U
#define DAQC_XRCE_MTU 128U
#define DAQC_FRAME_MAX 261U

typedef struct {
    uint8_t mid;
    uint8_t command;
    uint16_t sequence;
    const uint8_t *payload;
    size_t payload_size;
} daqc_frame_t;

typedef bool (*daqc_frame_handler_t)(const daqc_frame_t *frame, void *context);

typedef struct {
    uint8_t buffer[DAQC_FRAME_MAX];
    size_t buffered_size;
} daqc_parser_t;

void DaqcParserInit(daqc_parser_t *parser);
bool DaqcParserFeed(daqc_parser_t *parser, const uint8_t *bytes, size_t count, daqc_frame_handler_t handler, void *context);
size_t DaqcEncodeConfig(uint8_t *destination, size_t capacity, uint8_t command, bool include_status, uint8_t status);
size_t DaqcEncodeData(uint8_t *destination, size_t capacity, uint16_t sequence, const uint8_t *payload, size_t payload_size);
size_t DaqcEncodeReadAck(uint8_t *destination, size_t capacity, uint16_t sequence);
size_t DaqcEncodeXrce(uint8_t *destination, size_t capacity, const uint8_t *payload, size_t payload_size);

#endif
