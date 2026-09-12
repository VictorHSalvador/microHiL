/* Implementação do módulo daqc protocol. */
#include "daqc_protocol.h"

#include <string.h>

static uint16_t ReadLe16(const uint8_t *bytes) { return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U); }
static void WriteLe16(uint8_t *bytes, uint16_t value) { bytes[0] = (uint8_t)value; bytes[1] = (uint8_t)(value >> 8U); }

static void Discard(daqc_parser_t *parser, size_t count) {
    if (count >= parser->buffered_size) { parser->buffered_size = 0U; return; }
    memmove(parser->buffer, parser->buffer + count, parser->buffered_size - count);
    parser->buffered_size -= count;
}

static size_t FrameSize(const daqc_parser_t *parser) {
    if (parser->buffered_size < 3U) return 0U;
    if (parser->buffer[2] == DAQC_MID_CONFIG) return 4U;
    if (parser->buffer[2] == DAQC_MID_DATA) return 5U + DAQC_ACTUATION_SIZE;
    if (parser->buffer[2] == DAQC_MID_READ_ACK) return 5U;
    if (parser->buffer[2] == DAQC_MID_XRCE) return parser->buffered_size < 5U ? 0U : 5U + ReadLe16(parser->buffer + 3U);
    return 1U;
}

void DaqcParserInit(daqc_parser_t *parser) { if (parser) *parser = (daqc_parser_t){0}; }

bool DaqcParserFeed(daqc_parser_t *parser, const uint8_t *bytes, size_t count, daqc_frame_handler_t handler, void *context) {
    if (!parser || (count && !bytes) || !handler) return false;
    for (size_t index = 0U; index < count; ++index) {
        if (parser->buffered_size == sizeof(parser->buffer)) Discard(parser, 1U);
        parser->buffer[parser->buffered_size++] = bytes[index];
        while (parser->buffered_size >= 3U) {
            if (parser->buffer[0] != DAQC_SYNC_FIRST || parser->buffer[1] != DAQC_SYNC_SECOND) { Discard(parser, 1U); continue; }
            const size_t frame_size = FrameSize(parser);
            if (frame_size == 1U || frame_size > sizeof(parser->buffer)) { Discard(parser, 1U); continue; }
            if (!frame_size || parser->buffered_size < frame_size) break;
            daqc_frame_t frame = {.mid = parser->buffer[2]};
            if (frame.mid == DAQC_MID_CONFIG) frame.command = parser->buffer[3];
            if (frame.mid == DAQC_MID_DATA || frame.mid == DAQC_MID_READ_ACK) frame.sequence = ReadLe16(parser->buffer + 3U);
            if (frame.mid == DAQC_MID_DATA) { frame.payload = parser->buffer + 5U; frame.payload_size = DAQC_ACTUATION_SIZE; }
            if (frame.mid == DAQC_MID_XRCE) { frame.payload = parser->buffer + 5U; frame.payload_size = frame_size - 5U; }
            if (!handler(&frame, context)) return false;
            Discard(parser, frame_size);
        }
    }
    return true;
}

size_t DaqcEncodeConfig(uint8_t *destination, size_t capacity, uint8_t command, bool include_status, uint8_t status) {
    const size_t size = include_status ? 5U : 4U;
    if (!destination || capacity < size) return 0U;
    destination[0] = DAQC_SYNC_FIRST; destination[1] = DAQC_SYNC_SECOND; destination[2] = DAQC_MID_CONFIG; destination[3] = command;
    if (include_status) destination[4] = status;
    return size;
}
size_t DaqcEncodeData(uint8_t *destination, size_t capacity, uint16_t sequence, const uint8_t *payload, size_t payload_size) {
    if (!destination || !payload || payload_size != DAQC_ACQUISITION_SIZE || capacity < 5U + payload_size) return 0U;
    destination[0] = DAQC_SYNC_FIRST; destination[1] = DAQC_SYNC_SECOND; destination[2] = DAQC_MID_DATA; WriteLe16(destination + 3U, sequence); memcpy(destination + 5U, payload, payload_size); return 5U + payload_size;
}
size_t DaqcEncodeReadAck(uint8_t *destination, size_t capacity, uint16_t sequence) { if (!destination || capacity < 5U) return 0U; destination[0] = DAQC_SYNC_FIRST; destination[1] = DAQC_SYNC_SECOND; destination[2] = DAQC_MID_READ_ACK; WriteLe16(destination + 3U, sequence); return 5U; }
size_t DaqcEncodeXrce(uint8_t *destination, size_t capacity, const uint8_t *payload, size_t payload_size) { if (!destination || !payload || payload_size > DAQC_XRCE_MTU || capacity < 5U + payload_size) return 0U; destination[0] = DAQC_SYNC_FIRST; destination[1] = DAQC_SYNC_SECOND; destination[2] = DAQC_MID_XRCE; WriteLe16(destination + 3U, (uint16_t)payload_size); memcpy(destination + 5U, payload, payload_size); return 5U + payload_size; }
