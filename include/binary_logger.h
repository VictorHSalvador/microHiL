#ifndef BINARY_LOGGER_H
#define BINARY_LOGGER_H

#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "log_format.h"

#define BINARY_LOGGER_QUEUE_CAPACITY 128U
#define BINARY_LOGGER_ERROR_MESSAGE_SIZE 96U

typedef enum {
    BINARY_LOGGER_STATUS_OK = 0,
    BINARY_LOGGER_STATUS_INVALID_ARGUMENT,
    BINARY_LOGGER_STATUS_THREAD,
    BINARY_LOGGER_STATUS_SINK_IO,
    BINARY_LOGGER_STATUS_FORMAT,
    BINARY_LOGGER_STATUS_SINK_FAILURE
} binary_logger_status_t;

typedef enum {
    BINARY_LOGGER_STAGE_NONE = 0,
    BINARY_LOGGER_STAGE_OPEN,
    BINARY_LOGGER_STAGE_HEADER,
    BINARY_LOGGER_STAGE_RECORD,
    BINARY_LOGGER_STAGE_FLUSH,
    BINARY_LOGGER_STAGE_CLOSE
} binary_logger_stage_t;

typedef binary_logger_status_t (*binary_logger_open_fn)(void *context);
typedef binary_logger_status_t (*binary_logger_header_fn)(void *context, const log_descriptor_t *descriptor);
typedef binary_logger_status_t (*binary_logger_record_fn)(void *context, const log_descriptor_t *descriptor, const log_sample_t *sample);
typedef binary_logger_status_t (*binary_logger_flush_fn)(void *context);
typedef binary_logger_status_t (*binary_logger_close_fn)(void *context);
typedef const char *(*binary_logger_message_fn)(void *context, binary_logger_status_t status);

typedef struct {
    void *context;
    binary_logger_open_fn open;
    binary_logger_header_fn write_header;
    binary_logger_record_fn write_record;
    binary_logger_flush_fn flush;
    binary_logger_close_fn close;
    binary_logger_message_fn message;
} binary_logger_sink_t;

typedef struct {
    uint64_t accepted;
    uint64_t persisted;
    uint64_t discarded;
    bool has_last_persisted_sequence;
    uint64_t last_persisted_sequence;
    bool incomplete;
    binary_logger_stage_t first_error_stage;
    binary_logger_status_t first_error;
    char error_message[BINARY_LOGGER_ERROR_MESSAGE_SIZE];
} binary_logger_result_t;

typedef struct {
    const char *path;
    FILE *file;
    log_format_status_t format_status;
} binary_logger_file_sink_t;

/* Caller keeps the descriptor and sink context valid until join completes. */
typedef struct {
    pthread_t thread;
    const log_descriptor_t *descriptor;
    binary_logger_sink_t sink;
    _Atomic size_t head;
    _Atomic size_t tail;
    _Atomic uint64_t accepted;
    _Atomic uint64_t saturation_drops;
    _Atomic uint64_t empty_polls;
    _Atomic bool producer_done;
    bool started;
    /* The producer owns head and the consumer owns tail. */
    log_sample_t slots[BINARY_LOGGER_QUEUE_CAPACITY];
    binary_logger_result_t result;
} binary_logger_t;

void BinaryLoggerFileSinkInit(binary_logger_file_sink_t *file_sink, const char *path, binary_logger_sink_t *sink);
binary_logger_status_t BinaryLoggerStart(binary_logger_t *logger, const log_descriptor_t *descriptor, const binary_logger_sink_t *sink);
bool BinaryLoggerPublish(binary_logger_t *logger, const log_sample_t *sample);
void BinaryLoggerProducerDone(binary_logger_t *logger);
binary_logger_status_t BinaryLoggerJoin(binary_logger_t *logger, binary_logger_result_t *result);
uint64_t BinaryLoggerConsumerEmptyPolls(const binary_logger_t *logger);
const char *BinaryLoggerStatusString(binary_logger_status_t status);
const char *BinaryLoggerStageString(binary_logger_stage_t stage);

#endif
