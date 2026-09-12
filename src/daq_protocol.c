/* Implementação do módulo daq protocol. */
#include "daq_protocol.h"

#include <string.h>

static bool IsKnownMid(uint8_t mid) {
    return mid >= DAQ_PROTOCOL_MID_CONFIG && mid <= DAQ_PROTOCOL_MID_XRCE;
}

static uint16_t ReadLe16(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U);
}

static void WriteLe16(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)(value & UINT16_C(0x00ff));
    bytes[1] = (uint8_t)(value >> 8U);
}

static void DiscardPrefix(daq_protocol_parser_t *parser, size_t count) {
    if (count > parser->buffered_size) count = parser->buffered_size;
    if (count < parser->buffered_size) memmove(parser->buffer, parser->buffer + count, parser->buffered_size - count);
    parser->buffered_size -= count;
    parser->discarded_bytes += count;
}

static size_t FindSync(const daq_protocol_parser_t *parser) {
    if (parser->buffered_size < 2U) return 0U;
    for (size_t index = 0U; index + 1U < parser->buffered_size; ++index) {
        if (parser->buffer[index] == DAQ_PROTOCOL_SYNC_FIRST && parser->buffer[index + 1U] == DAQ_PROTOCOL_SYNC_SECOND) return index;
    }
    return parser->buffered_size - 1U;
}

static bool FrameLength(const daq_protocol_parser_t *parser, size_t *frame_size) {
    if (parser->buffered_size < DAQ_PROTOCOL_HEADER_SIZE) return false;
    switch (parser->buffer[2]) {
        case DAQ_PROTOCOL_MID_CONFIG:
            *frame_size = parser->config_has_status ? 5U : 4U;
            return true;
        case DAQ_PROTOCOL_MID_DATA:
            *frame_size = DAQ_PROTOCOL_DATA_PREFIX_SIZE + parser->data_payload_size;
            return true;
        case DAQ_PROTOCOL_MID_READ_ACK:
            *frame_size = 5U;
            return true;
        case DAQ_PROTOCOL_MID_XRCE:
            if (parser->buffered_size < 5U) return false;
            *frame_size = 5U + ReadLe16(parser->buffer + 3U);
            return true;
        default:
            return false;
    }
}

static bool EmitFrame(const daq_protocol_parser_t *parser, size_t frame_size, daq_protocol_frame_callback_t callback, void *context) {
    daq_protocol_frame_t frame = {.mid = (daq_protocol_mid_t)parser->buffer[2]};
    switch (frame.mid) {
        case DAQ_PROTOCOL_MID_CONFIG:
            frame.command = parser->buffer[3];
            frame.has_status = parser->config_has_status;
            if (frame.has_status) frame.status = parser->buffer[4];
            break;
        case DAQ_PROTOCOL_MID_DATA:
            frame.sequence = ReadLe16(parser->buffer + 3U);
            frame.has_sequence = true;
            frame.payload = parser->buffer + DAQ_PROTOCOL_DATA_PREFIX_SIZE;
            frame.payload_size = frame_size - DAQ_PROTOCOL_DATA_PREFIX_SIZE;
            break;
        case DAQ_PROTOCOL_MID_READ_ACK:
            frame.sequence = ReadLe16(parser->buffer + 3U);
            frame.has_sequence = true;
            break;
        case DAQ_PROTOCOL_MID_XRCE:
            frame.payload = parser->buffer + 5U;
            frame.payload_size = frame_size - 5U;
            break;
        default:
            return false;
    }
    return callback(&frame, context);
}

bool DaqProtocolParserInit(daq_protocol_parser_t *parser, uint8_t *buffer, size_t buffer_capacity, size_t data_payload_size, bool config_has_status) {
    if (!parser || !buffer || data_payload_size > DAQ_PROTOCOL_MAX_DATA_PAYLOAD || buffer_capacity < DAQ_PROTOCOL_DATA_PREFIX_SIZE + data_payload_size) return false;
    *parser = (daq_protocol_parser_t){.buffer = buffer, .buffer_capacity = buffer_capacity, .data_payload_size = data_payload_size,
                                      .config_has_status = config_has_status};
    return true;
}

daq_protocol_parse_status_t DaqProtocolParserFeed(daq_protocol_parser_t *parser, const uint8_t *bytes, size_t byte_count, daq_protocol_frame_callback_t callback, void *context) {
    if (!parser || !parser->buffer || (byte_count > 0U && !bytes) || !callback) return DAQ_PROTOCOL_PARSE_INVALID_ARGUMENT;

    for (size_t input_index = 0U; input_index < byte_count; ++input_index) {
        if (parser->buffered_size == parser->buffer_capacity) {
            DiscardPrefix(parser, 1U);
            ++parser->invalid_frames;
        }
        parser->buffer[parser->buffered_size++] = bytes[input_index];

        for (;;) {
            const size_t sync_index = FindSync(parser);
            if (sync_index > 0U) {
                DiscardPrefix(parser, sync_index);
                if (parser->buffered_size < 2U) break;
            }
            if (parser->buffered_size < DAQ_PROTOCOL_HEADER_SIZE) break;
            if (!IsKnownMid(parser->buffer[2])) {
                DiscardPrefix(parser, 1U);
                ++parser->invalid_frames;
                continue;
            }

            size_t frame_size = 0U;
            if (!FrameLength(parser, &frame_size)) break;
            if (frame_size > parser->buffer_capacity) {
                DiscardPrefix(parser, 1U);
                ++parser->invalid_frames;
                continue;
            }
            if (parser->buffered_size < frame_size) break;
            if (!EmitFrame(parser, frame_size, callback, context)) return DAQ_PROTOCOL_PARSE_CALLBACK_STOPPED;
            DiscardPrefix(parser, frame_size);
        }
    }
    return DAQ_PROTOCOL_PARSE_OK;
}

size_t DaqProtocolEncodeConfig(uint8_t *destination, size_t capacity, daq_protocol_command_t command, bool include_status, uint8_t status) {
    const size_t frame_size = include_status ? 5U : 4U;
    if (!destination || capacity < frame_size || command < DAQ_PROTOCOL_COMMAND_DISABLE || command > DAQ_PROTOCOL_COMMAND_STREAMING) return 0U;
    destination[0] = DAQ_PROTOCOL_SYNC_FIRST;
    destination[1] = DAQ_PROTOCOL_SYNC_SECOND;
    destination[2] = DAQ_PROTOCOL_MID_CONFIG;
    destination[3] = (uint8_t)command;
    if (include_status) destination[4] = status;
    return frame_size;
}

size_t DaqProtocolEncodeData(uint8_t *destination, size_t capacity, uint16_t sequence, const uint8_t *payload, size_t payload_size) {
    const size_t frame_size = DAQ_PROTOCOL_DATA_PREFIX_SIZE + payload_size;
    if (!destination || (payload_size > 0U && !payload) || payload_size > DAQ_PROTOCOL_MAX_DATA_PAYLOAD || capacity < frame_size) return 0U;
    destination[0] = DAQ_PROTOCOL_SYNC_FIRST;
    destination[1] = DAQ_PROTOCOL_SYNC_SECOND;
    destination[2] = DAQ_PROTOCOL_MID_DATA;
    WriteLe16(destination + 3U, sequence);
    if (payload_size > 0U) memcpy(destination + DAQ_PROTOCOL_DATA_PREFIX_SIZE, payload, payload_size);
    return frame_size;
}

size_t DaqProtocolEncodeReadAck(uint8_t *destination, size_t capacity, uint16_t sequence) {
    if (!destination || capacity < 5U) return 0U;
    destination[0] = DAQ_PROTOCOL_SYNC_FIRST;
    destination[1] = DAQ_PROTOCOL_SYNC_SECOND;
    destination[2] = DAQ_PROTOCOL_MID_READ_ACK;
    WriteLe16(destination + 3U, sequence);
    return 5U;
}

size_t DaqProtocolEncodeXrce(uint8_t *destination, size_t capacity, const uint8_t *payload, size_t payload_size) {
    if (!destination || (payload_size > 0U && !payload) || payload_size > DAQ_PROTOCOL_XRCE_MTU || capacity < 5U + payload_size) return 0U;
    destination[0] = DAQ_PROTOCOL_SYNC_FIRST;
    destination[1] = DAQ_PROTOCOL_SYNC_SECOND;
    destination[2] = DAQ_PROTOCOL_MID_XRCE;
    WriteLe16(destination + 3U, (uint16_t)payload_size);
    if (payload_size > 0U) memcpy(destination + 5U, payload, payload_size);
    return 5U + payload_size;
}

daq_sequence_relation_t DaqSequenceTrack(daq_sequence_tracker_t *tracker, uint16_t sequence) {
    if (!tracker) return DAQ_SEQUENCE_OLD;
    if (!tracker->initialized) {
        tracker->initialized = true;
        tracker->last_sequence = sequence;
        return DAQ_SEQUENCE_FIRST;
    }

    const uint16_t delta = (uint16_t)(sequence - tracker->last_sequence);
    if (delta == 0U) {
        ++tracker->duplicate_frames;
        return DAQ_SEQUENCE_DUPLICATE;
    }
    if (delta > UINT16_C(0x7fff)) {
        ++tracker->old_frames;
        return DAQ_SEQUENCE_OLD;
    }
    tracker->last_sequence = sequence;
    if (delta == 1U) return DAQ_SEQUENCE_EXPECTED;
    tracker->missing_frames += (uint64_t)delta - 1U;
    return DAQ_SEQUENCE_GAP;
}
