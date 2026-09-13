#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daqc_protocol.h"

static size_t g_frame_count;
static daqc_frame_t g_frame;

static void Require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static bool CaptureFrame(const daqc_frame_t *frame, void *context) {
    (void)context;
    g_frame_count++;
    g_frame = *frame;
    return true;
}

int main(void) {
    daqc_parser_t parser;
    uint8_t data[5U + DAQC_ACTUATION_SIZE] = {DAQC_SYNC_FIRST, DAQC_SYNC_SECOND, DAQC_MID_DATA, 7U, 0U};
    data[9] = DAQC_SYNC_FIRST;
    data[10] = DAQC_SYNC_SECOND;
    DaqcParserInit(&parser);
    Require(DaqcParserFeed(&parser, data, sizeof(data), CaptureFrame, NULL), "could not parse complete DATA frame");
    Require(g_frame_count == 1U && g_frame.mid == DAQC_MID_DATA && g_frame.payload_size == DAQC_ACTUATION_SIZE,
            "DATA frame was not kept at its ICD size when payload contains SYNC");

    const uint8_t oversize_then_config[] = {DAQC_SYNC_FIRST, DAQC_SYNC_SECOND, DAQC_MID_XRCE, 129U, 0U,
                                             DAQC_SYNC_FIRST, DAQC_SYNC_SECOND, DAQC_MID_CONFIG, DAQC_COMMAND_DISABLE};
    g_frame_count = 0U;
    DaqcParserInit(&parser);
    Require(DaqcParserFeed(&parser, oversize_then_config, sizeof(oversize_then_config), CaptureFrame, NULL),
            "parser rejected recoverable oversize XRCE");
    Require(g_frame_count == 1U && g_frame.mid == DAQC_MID_CONFIG && g_frame.command == DAQC_COMMAND_DISABLE,
            "parser did not recover after oversize XRCE");
    return EXIT_SUCCESS;
}
