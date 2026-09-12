#ifndef DAQ_PROTOCOL_H
#define DAQ_PROTOCOL_H

/* Contrato público do módulo daq protocol. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DAQ_PROTOCOL_SYNC_FIRST UINT8_C(0x59)
#define DAQ_PROTOCOL_SYNC_SECOND UINT8_C(0x72)
#define DAQ_PROTOCOL_MAX_DATA_PAYLOAD 256U
#define DAQ_PROTOCOL_XRCE_MTU 128U
#define DAQ_PROTOCOL_HEADER_SIZE 3U
#define DAQ_PROTOCOL_DATA_PREFIX_SIZE 5U

typedef enum {
    DAQ_PROTOCOL_MID_CONFIG = 0x01,
    DAQ_PROTOCOL_MID_DATA = 0x02,
    DAQ_PROTOCOL_MID_READ_ACK = 0x03,
    DAQ_PROTOCOL_MID_XRCE = 0x04
} daq_protocol_mid_t;

typedef enum {
    DAQ_PROTOCOL_COMMAND_DISABLE = 0x01,
    DAQ_PROTOCOL_COMMAND_ENABLE = 0x02,
    DAQ_PROTOCOL_COMMAND_STREAMING = 0x03
} daq_protocol_command_t;

typedef struct {
    daq_protocol_mid_t mid;
    const uint8_t *payload;
    size_t payload_size;
    uint16_t sequence;
    uint8_t command;
    uint8_t status;
    bool has_sequence;
    bool has_status;
} daq_protocol_frame_t;

typedef enum {
    DAQ_PROTOCOL_PARSE_OK = 0,
    DAQ_PROTOCOL_PARSE_INVALID_ARGUMENT,
    DAQ_PROTOCOL_PARSE_BUFFER_TOO_SMALL,
    DAQ_PROTOCOL_PARSE_INVALID_CONFIGURATION,
    DAQ_PROTOCOL_PARSE_CALLBACK_STOPPED
} daq_protocol_parse_status_t;

typedef bool (*daq_protocol_frame_callback_t)(const daq_protocol_frame_t *frame, void *context);

typedef struct {
    uint8_t *buffer;
    size_t buffer_capacity;
    size_t buffered_size;
    size_t data_payload_size;
    bool config_has_status;
    uint64_t discarded_bytes;
    uint64_t invalid_frames;
} daq_protocol_parser_t;

typedef enum {
    DAQ_SEQUENCE_FIRST = 0,
    DAQ_SEQUENCE_EXPECTED,
    DAQ_SEQUENCE_GAP,
    DAQ_SEQUENCE_DUPLICATE,
    DAQ_SEQUENCE_OLD
} daq_sequence_relation_t;

typedef struct {
    bool initialized;
    uint16_t last_sequence;
    uint64_t missing_frames;
    uint64_t duplicate_frames;
    uint64_t old_frames;
} daq_sequence_tracker_t;

bool DaqProtocolParserInit(daq_protocol_parser_t *parser, uint8_t *buffer, size_t buffer_capacity, size_t data_payload_size, bool config_has_status);
daq_protocol_parse_status_t DaqProtocolParserFeed(daq_protocol_parser_t *parser, const uint8_t *bytes, size_t byte_count, daq_protocol_frame_callback_t callback, void *context);
size_t DaqProtocolEncodeConfig(uint8_t *destination, size_t capacity, daq_protocol_command_t command, bool include_status, uint8_t status);
size_t DaqProtocolEncodeData(uint8_t *destination, size_t capacity, uint16_t sequence, const uint8_t *payload, size_t payload_size);
size_t DaqProtocolEncodeReadAck(uint8_t *destination, size_t capacity, uint16_t sequence);
size_t DaqProtocolEncodeXrce(uint8_t *destination, size_t capacity, const uint8_t *payload, size_t payload_size);
daq_sequence_relation_t DaqSequenceTrack(daq_sequence_tracker_t *tracker, uint16_t sequence);

#endif
