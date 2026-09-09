#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daq_protocol.h"

typedef struct {
    size_t count;
    daq_protocol_frame_t frames[4];
    uint8_t payloads[4][DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
} frame_capture_t;

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static bool CaptureFrame(const daq_protocol_frame_t *frame, void *context) {
    frame_capture_t *capture = context;
    if (capture->count == 4U) return false;
    capture->frames[capture->count] = *frame;
    if (frame->payload_size > 0U) {
        memcpy(capture->payloads[capture->count], frame->payload, frame->payload_size);
        capture->frames[capture->count].payload = capture->payloads[capture->count];
    }
    ++capture->count;
    return true;
}

static void TestFrames(void) {
    uint8_t parser_buffer[64];
    uint8_t bytes[64];
    uint8_t payload[] = {UINT8_C(0x59), UINT8_C(0x72), UINT8_C(0xa5)};
    frame_capture_t capture = {0};
    daq_protocol_parser_t parser;

    Require(DaqProtocolEncodeConfig(bytes, sizeof(bytes), DAQ_PROTOCOL_COMMAND_STREAMING, false, 0U) == 4U, "could not encode CONFIG");
    Require(DaqProtocolEncodeData(bytes + 4U, sizeof(bytes) - 4U, 7U, payload, sizeof(payload)) == 8U, "could not encode DATA");
    Require(DaqProtocolEncodeReadAck(bytes + 12U, sizeof(bytes) - 12U, 7U) == 5U, "could not encode READ_ACK");
    Require(DaqProtocolParserInit(&parser, parser_buffer, sizeof(parser_buffer), sizeof(payload), false), "could not initialize parser");
    Require(DaqProtocolParserFeed(&parser, bytes, 17U, CaptureFrame, &capture) == DAQ_PROTOCOL_PARSE_OK, "could not parse frames");
    Require(capture.count == 3U, "parser did not emit every aggregate frame");
    Require(capture.frames[0].mid == DAQ_PROTOCOL_MID_CONFIG && capture.frames[0].command == DAQ_PROTOCOL_COMMAND_STREAMING,
            "CONFIG content differs");
    Require(capture.frames[1].mid == DAQ_PROTOCOL_MID_DATA && capture.frames[1].sequence == 7U && capture.frames[1].payload_size == sizeof(payload) &&
            memcmp(capture.frames[1].payload, payload, sizeof(payload)) == 0, "DATA content differs");
    Require(capture.frames[2].mid == DAQ_PROTOCOL_MID_READ_ACK && capture.frames[2].sequence == 7U, "READ_ACK content differs");
}

static void TestFragmentationAndResync(void) {
    uint8_t parser_buffer[64];
    uint8_t bytes[16];
    uint8_t payload[] = {UINT8_C(0x01), UINT8_C(0x02)};
    frame_capture_t capture = {0};
    daq_protocol_parser_t parser;

    Require(DaqProtocolEncodeData(bytes + 3U, sizeof(bytes) - 3U, UINT16_C(0xffff), payload, sizeof(payload)) == 7U, "could not encode wrapped DATA");
    bytes[0] = UINT8_C(0x99);
    bytes[1] = DAQ_PROTOCOL_SYNC_FIRST;
    bytes[2] = UINT8_C(0x33);
    Require(DaqProtocolParserInit(&parser, parser_buffer, sizeof(parser_buffer), sizeof(payload), false), "could not initialize parser");
    Require(DaqProtocolParserFeed(&parser, bytes, 6U, CaptureFrame, &capture) == DAQ_PROTOCOL_PARSE_OK && capture.count == 0U,
            "fragment must not be emitted early");
    Require(DaqProtocolParserFeed(&parser, bytes + 6U, 4U, CaptureFrame, &capture) == DAQ_PROTOCOL_PARSE_OK && capture.count == 1U,
            "fragmented DATA was not recovered after resync");
    Require(capture.frames[0].sequence == UINT16_C(0xffff) && parser.discarded_bytes >= 2U, "resync metrics or wrapped sequence differ");
}

static void TestSequence(void) {
    daq_sequence_tracker_t tracker = {0};
    Require(DaqSequenceTrack(&tracker, UINT16_C(0xffff)) == DAQ_SEQUENCE_FIRST, "first sequence not accepted");
    Require(DaqSequenceTrack(&tracker, 0U) == DAQ_SEQUENCE_EXPECTED, "wrap sequence not accepted");
    Require(DaqSequenceTrack(&tracker, 3U) == DAQ_SEQUENCE_GAP && tracker.missing_frames == 2U, "gap was not counted");
    Require(DaqSequenceTrack(&tracker, 3U) == DAQ_SEQUENCE_DUPLICATE && tracker.duplicate_frames == 1U, "duplicate was not counted");
    Require(DaqSequenceTrack(&tracker, 2U) == DAQ_SEQUENCE_OLD && tracker.old_frames == 1U, "old sequence was not counted");
}

int main(int argc, char **argv) {
    if (argc != 2) return EXIT_FAILURE;
    if (strcmp(argv[1], "frames") == 0) TestFrames();
    else if (strcmp(argv[1], "fragmentation") == 0) TestFragmentationAndResync();
    else if (strcmp(argv[1], "sequence") == 0) TestSequence();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
