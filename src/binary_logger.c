#include "binary_logger.h"

#include <string.h>
#include <time.h>

static void CopyErrorMessage(binary_logger_result_t *result, const char *message) {
    size_t length = 0U;

    if (!message) return;
    while (length + 1U < sizeof(result->error_message) && message[length] != '\0') {
        result->error_message[length] = message[length];
        ++length;
    }
    result->error_message[length] = '\0';
}

static void RememberError(binary_logger_t *logger, binary_logger_stage_t stage, binary_logger_status_t status) {
    if (logger->result.first_error_stage != BINARY_LOGGER_STAGE_NONE) return;

    logger->result.incomplete = true;
    logger->result.first_error_stage = stage;
    logger->result.first_error = status;
    CopyErrorMessage(&logger->result, logger->sink.message ? logger->sink.message(logger->sink.context, status) : BinaryLoggerStatusString(status));
}

static bool PopSample(binary_logger_t *logger, log_sample_t *sample) {
    const size_t tail = atomic_load_explicit(&logger->tail, memory_order_relaxed);
    const size_t head = atomic_load_explicit(&logger->head, memory_order_acquire);

    if (tail == head) return false;
    *sample = logger->slots[tail];
    atomic_store_explicit(&logger->tail, (tail + 1U) % BINARY_LOGGER_QUEUE_CAPACITY, memory_order_release);
    return true;
}

static void SleepBriefly(void) {
    const struct timespec delay = {.tv_sec = 0, .tv_nsec = 1000000L};

    (void)nanosleep(&delay, NULL);
}

static void FinalizeSink(binary_logger_t *logger, bool sink_open) {
    binary_logger_status_t status;

    if (!sink_open) return;
    status = logger->sink.flush(logger->sink.context);
    if (status != BINARY_LOGGER_STATUS_OK) RememberError(logger, BINARY_LOGGER_STAGE_FLUSH, status);
    status = logger->sink.close(logger->sink.context);
    if (status != BINARY_LOGGER_STATUS_OK) RememberError(logger, BINARY_LOGGER_STAGE_CLOSE, status);
}

static void *BinaryLoggerThread(void *argument) {
    binary_logger_t *logger = argument;
    bool sink_open = false;
    bool sink_usable = true;
    binary_logger_status_t status = logger->sink.open(logger->sink.context);

    if (status != BINARY_LOGGER_STATUS_OK) {
        RememberError(logger, BINARY_LOGGER_STAGE_OPEN, status);
        sink_usable = false;
    } else {
        sink_open = true;
        status = logger->sink.write_header(logger->sink.context, logger->descriptor);
        if (status != BINARY_LOGGER_STATUS_OK) {
            RememberError(logger, BINARY_LOGGER_STAGE_HEADER, status);
            sink_usable = false;
        }
    }

    for (;;) {
        log_sample_t sample;

        while (PopSample(logger, &sample)) {
            if (!sink_usable) {
                ++logger->result.discarded;
                continue;
            }
            status = logger->sink.write_record(logger->sink.context, logger->descriptor, &sample);
            if (status != BINARY_LOGGER_STATUS_OK) {
                RememberError(logger, BINARY_LOGGER_STAGE_RECORD, status);
                sink_usable = false;
                ++logger->result.discarded;
                continue;
            }
            ++logger->result.persisted;
            logger->result.has_last_persisted_sequence = true;
            logger->result.last_persisted_sequence = sample.sequence;
        }

        atomic_fetch_add_explicit(&logger->empty_polls, 1U, memory_order_relaxed);
        if (atomic_load_explicit(&logger->producer_done, memory_order_acquire)) {
            if (!PopSample(logger, &sample)) break;
            if (!sink_usable) {
                ++logger->result.discarded;
            } else {
                status = logger->sink.write_record(logger->sink.context, logger->descriptor, &sample);
                if (status != BINARY_LOGGER_STATUS_OK) {
                    RememberError(logger, BINARY_LOGGER_STAGE_RECORD, status);
                    sink_usable = false;
                    ++logger->result.discarded;
                } else {
                    ++logger->result.persisted;
                    logger->result.has_last_persisted_sequence = true;
                    logger->result.last_persisted_sequence = sample.sequence;
                }
            }
            continue;
        }
        SleepBriefly();
    }

    FinalizeSink(logger, sink_open);
    logger->result.accepted = atomic_load_explicit(&logger->accepted, memory_order_relaxed);
    logger->result.discarded += atomic_load_explicit(&logger->saturation_drops, memory_order_relaxed);
    if (logger->result.discarded != 0U) logger->result.incomplete = true;
    return NULL;
}

static binary_logger_status_t FileOpen(void *context) {
    binary_logger_file_sink_t *file_sink = context;

    if (!file_sink || !file_sink->path) return BINARY_LOGGER_STATUS_INVALID_ARGUMENT;
    file_sink->file = fopen(file_sink->path, "wb");
    return file_sink->file ? BINARY_LOGGER_STATUS_OK : BINARY_LOGGER_STATUS_SINK_IO;
}

static binary_logger_status_t FileWriteHeader(void *context, const log_descriptor_t *descriptor) {
    binary_logger_file_sink_t *file_sink = context;

    file_sink->format_status = LogFormatWriteHeader(file_sink->file, descriptor);
    return file_sink->format_status == LOG_FORMAT_STATUS_OK ? BINARY_LOGGER_STATUS_OK : BINARY_LOGGER_STATUS_FORMAT;
}

static binary_logger_status_t FileWriteRecord(void *context, const log_descriptor_t *descriptor, const log_sample_t *sample) {
    binary_logger_file_sink_t *file_sink = context;

    file_sink->format_status = LogFormatWriteRecord(file_sink->file, descriptor, sample);
    return file_sink->format_status == LOG_FORMAT_STATUS_OK ? BINARY_LOGGER_STATUS_OK : BINARY_LOGGER_STATUS_FORMAT;
}

static binary_logger_status_t FileFlush(void *context) {
    binary_logger_file_sink_t *file_sink = context;

    return fflush(file_sink->file) == 0 ? BINARY_LOGGER_STATUS_OK : BINARY_LOGGER_STATUS_SINK_IO;
}

static binary_logger_status_t FileClose(void *context) {
    binary_logger_file_sink_t *file_sink = context;
    const int close_status = fclose(file_sink->file);

    file_sink->file = NULL;
    return close_status == 0 ? BINARY_LOGGER_STATUS_OK : BINARY_LOGGER_STATUS_SINK_IO;
}

static const char *FileMessage(void *context, binary_logger_status_t status) {
    const binary_logger_file_sink_t *file_sink = context;

    if (status == BINARY_LOGGER_STATUS_FORMAT) return LogFormatStatusString(file_sink->format_status);
    return BinaryLoggerStatusString(status);
}

void BinaryLoggerFileSinkInit(binary_logger_file_sink_t *file_sink, const char *path, binary_logger_sink_t *sink) {
    if (!file_sink || !sink) return;
    *file_sink = (binary_logger_file_sink_t){.path = path, .file = NULL, .format_status = LOG_FORMAT_STATUS_OK};
    *sink = (binary_logger_sink_t){
        .context = file_sink,
        .open = FileOpen,
        .write_header = FileWriteHeader,
        .write_record = FileWriteRecord,
        .flush = FileFlush,
        .close = FileClose,
        .message = FileMessage
    };
}

binary_logger_status_t BinaryLoggerStart(binary_logger_t *logger, const log_descriptor_t *descriptor, const binary_logger_sink_t *sink) {
    if (!logger || !descriptor || !sink || !sink->open || !sink->write_header || !sink->write_record || !sink->flush || !sink->close) {
        return BINARY_LOGGER_STATUS_INVALID_ARGUMENT;
    }
    *logger = (binary_logger_t){.descriptor = descriptor, .sink = *sink, .started = false};
    atomic_init(&logger->head, 0U);
    atomic_init(&logger->tail, 0U);
    atomic_init(&logger->accepted, UINT64_C(0));
    atomic_init(&logger->saturation_drops, UINT64_C(0));
    atomic_init(&logger->empty_polls, UINT64_C(0));
    atomic_init(&logger->producer_done, false);
    if (pthread_create(&logger->thread, NULL, BinaryLoggerThread, logger) != 0) return BINARY_LOGGER_STATUS_THREAD;
    logger->started = true;
    return BINARY_LOGGER_STATUS_OK;
}

bool BinaryLoggerPublish(binary_logger_t *logger, const log_sample_t *sample) {
    size_t head;
    size_t next;
    size_t tail;

    if (!logger || !sample || !logger->started) return false;
    if (atomic_load_explicit(&logger->producer_done, memory_order_acquire)) return false;
    head = atomic_load_explicit(&logger->head, memory_order_relaxed);
    next = (head + 1U) % BINARY_LOGGER_QUEUE_CAPACITY;
    tail = atomic_load_explicit(&logger->tail, memory_order_acquire);
    if (next == tail) {
        atomic_fetch_add_explicit(&logger->saturation_drops, UINT64_C(1), memory_order_relaxed);
        return false;
    }
    logger->slots[head] = *sample;
    atomic_store_explicit(&logger->head, next, memory_order_release);
    atomic_fetch_add_explicit(&logger->accepted, UINT64_C(1), memory_order_relaxed);
    return true;
}

void BinaryLoggerProducerDone(binary_logger_t *logger) {
    if (!logger) return;
    atomic_store_explicit(&logger->producer_done, true, memory_order_release);
}

binary_logger_status_t BinaryLoggerJoin(binary_logger_t *logger, binary_logger_result_t *result) {
    if (!logger || !logger->started) return BINARY_LOGGER_STATUS_INVALID_ARGUMENT;
    if (pthread_join(logger->thread, NULL) != 0) return BINARY_LOGGER_STATUS_THREAD;
    logger->started = false;
    if (result) *result = logger->result;
    return BINARY_LOGGER_STATUS_OK;
}

uint64_t BinaryLoggerConsumerEmptyPolls(const binary_logger_t *logger) {
    return logger ? atomic_load_explicit(&logger->empty_polls, memory_order_relaxed) : UINT64_C(0);
}

const char *BinaryLoggerStatusString(binary_logger_status_t status) {
    switch (status) {
        case BINARY_LOGGER_STATUS_OK: return "ok";
        case BINARY_LOGGER_STATUS_INVALID_ARGUMENT: return "invalid argument";
        case BINARY_LOGGER_STATUS_THREAD: return "thread error";
        case BINARY_LOGGER_STATUS_SINK_IO: return "sink I/O error";
        case BINARY_LOGGER_STATUS_FORMAT: return "log format error";
        case BINARY_LOGGER_STATUS_SINK_FAILURE: return "sink failure";
        default: return "unknown logger status";
    }
}

const char *BinaryLoggerStageString(binary_logger_stage_t stage) {
    switch (stage) {
        case BINARY_LOGGER_STAGE_NONE: return "none";
        case BINARY_LOGGER_STAGE_OPEN: return "open";
        case BINARY_LOGGER_STAGE_HEADER: return "header";
        case BINARY_LOGGER_STAGE_RECORD: return "record";
        case BINARY_LOGGER_STAGE_FLUSH: return "flush";
        case BINARY_LOGGER_STAGE_CLOSE: return "close";
        default: return "unknown logger stage";
    }
}
