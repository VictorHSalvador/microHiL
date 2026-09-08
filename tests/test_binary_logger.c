#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "binary_logger.h"

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool block_open;
    bool open_entered;
    bool release_open;
    bool producer_thread_set;
    pthread_t producer_thread;
    binary_logger_stage_t failure_stage;
    uint64_t sequences[BINARY_LOGGER_QUEUE_CAPACITY * 5U];
    size_t open_calls;
    size_t header_calls;
    size_t record_calls;
    size_t flush_calls;
    size_t close_calls;
    size_t producer_callback_calls;
} test_sink_t;

typedef struct {
    binary_logger_t *logger;
    test_sink_t *sink;
    uint64_t first_sequence;
    size_t count;
    size_t accepted;
    _Atomic bool finished;
} producer_context_t;

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void RequireStatus(binary_logger_status_t actual, binary_logger_status_t expected, const char *message) {
    if (actual != expected) {
        fprintf(stderr, "%s: got %s\n", message, BinaryLoggerStatusString(actual));
        exit(EXIT_FAILURE);
    }
}

static log_descriptor_t MakeDescriptor(void) {
    log_descriptor_t descriptor = {0};

    descriptor.output_count = 1U;
    descriptor.step_size_s = 0.01;
    descriptor.outputs[0] = (log_output_t){.xml_index = 1U, .value_reference = 3U, .type = LOG_VALUE_REAL, .name = "output"};
    return descriptor;
}

static log_sample_t MakeSample(uint64_t sequence) {
    log_sample_t sample = {0};

    sample.sequence = sequence;
    sample.sim_time_s = (double)sequence * 0.01;
    sample.values[0].real_value = (double)sequence;
    return sample;
}

static void InitTestSink(test_sink_t *sink) {
    *sink = (test_sink_t){0};
    Require(pthread_mutex_init(&sink->mutex, NULL) == 0, "could not initialize test sink mutex");
    Require(pthread_cond_init(&sink->condition, NULL) == 0, "could not initialize test sink condition");
}

static void DestroyTestSink(test_sink_t *sink) {
    Require(pthread_cond_destroy(&sink->condition) == 0, "could not destroy test sink condition");
    Require(pthread_mutex_destroy(&sink->mutex) == 0, "could not destroy test sink mutex");
}

static void NoteCallback(test_sink_t *sink) {
    if (sink->producer_thread_set && pthread_equal(pthread_self(), sink->producer_thread) != 0) ++sink->producer_callback_calls;
}

static void SetProducerThread(test_sink_t *sink) {
    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    sink->producer_thread = pthread_self();
    sink->producer_thread_set = true;
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
}

static binary_logger_status_t TestOpen(void *context) {
    test_sink_t *sink = context;

    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    NoteCallback(sink);
    ++sink->open_calls;
    sink->open_entered = true;
    Require(pthread_cond_broadcast(&sink->condition) == 0, "could not signal test sink");
    while (sink->block_open && !sink->release_open) Require(pthread_cond_wait(&sink->condition, &sink->mutex) == 0, "could not wait in open callback");
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
    return sink->failure_stage == BINARY_LOGGER_STAGE_OPEN ? BINARY_LOGGER_STATUS_SINK_FAILURE : BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t TestHeader(void *context, const log_descriptor_t *descriptor) {
    test_sink_t *sink = context;

    (void)descriptor;
    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    NoteCallback(sink);
    ++sink->header_calls;
    Require(pthread_cond_broadcast(&sink->condition) == 0, "could not signal test sink");
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
    return sink->failure_stage == BINARY_LOGGER_STAGE_HEADER ? BINARY_LOGGER_STATUS_SINK_FAILURE : BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t TestRecord(void *context, const log_descriptor_t *descriptor, const log_sample_t *sample) {
    test_sink_t *sink = context;

    (void)descriptor;
    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    NoteCallback(sink);
    Require(sink->record_calls < sizeof(sink->sequences) / sizeof(sink->sequences[0]), "test sink record capacity exceeded");
    sink->sequences[sink->record_calls++] = sample->sequence;
    Require(pthread_cond_broadcast(&sink->condition) == 0, "could not signal test sink");
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
    return sink->failure_stage == BINARY_LOGGER_STAGE_RECORD ? BINARY_LOGGER_STATUS_SINK_FAILURE : BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t TestFlush(void *context) {
    test_sink_t *sink = context;

    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    NoteCallback(sink);
    ++sink->flush_calls;
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
    return sink->failure_stage == BINARY_LOGGER_STAGE_FLUSH ? BINARY_LOGGER_STATUS_SINK_FAILURE : BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t TestClose(void *context) {
    test_sink_t *sink = context;

    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    NoteCallback(sink);
    ++sink->close_calls;
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
    return sink->failure_stage == BINARY_LOGGER_STAGE_CLOSE ? BINARY_LOGGER_STATUS_SINK_FAILURE : BINARY_LOGGER_STATUS_OK;
}

static const char *TestMessage(void *context, binary_logger_status_t status) {
    (void)context;
    return status == BINARY_LOGGER_STATUS_SINK_FAILURE ? "deterministic sink failure" : BinaryLoggerStatusString(status);
}

static binary_logger_sink_t MakeTestSink(test_sink_t *test_sink) {
    return (binary_logger_sink_t){
        .context = test_sink,
        .open = TestOpen,
        .write_header = TestHeader,
        .write_record = TestRecord,
        .flush = TestFlush,
        .close = TestClose,
        .message = TestMessage
    };
}

static bool WaitForCondition(bool (*predicate)(void *context), void *context) {
    struct timespec start;
    struct timespec now;

    Require(clock_gettime(CLOCK_MONOTONIC, &start) == 0, "could not read monotonic clock");
    for (;;) {
        if (predicate(context)) return true;
        Require(clock_gettime(CLOCK_MONOTONIC, &now) == 0, "could not read monotonic clock");
        if (now.tv_sec - start.tv_sec >= 2) return false;
        sched_yield();
    }
}

static bool IsOpenEntered(void *context) {
    test_sink_t *sink = context;
    bool entered;

    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    entered = sink->open_entered;
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
    return entered;
}

static bool IsProducerFinished(void *context) {
    const producer_context_t *producer = context;

    return atomic_load_explicit(&producer->finished, memory_order_acquire);
}

static void *PublishSamples(void *argument) {
    producer_context_t *producer = argument;

    SetProducerThread(producer->sink);
    for (size_t index = 0U; index < producer->count; ++index) {
        const log_sample_t sample = MakeSample(producer->first_sequence + (uint64_t)index);

        if (BinaryLoggerPublish(producer->logger, &sample)) ++producer->accepted;
    }
    BinaryLoggerProducerDone(producer->logger);
    atomic_store_explicit(&producer->finished, true, memory_order_release);
    return NULL;
}

/* @spec:AC-018 */
static void TestSaturationDoesNotBlockProducer(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t test_sink;
    const binary_logger_sink_t sink = MakeTestSink(&test_sink);
    binary_logger_t logger;
    binary_logger_result_t result;
    producer_context_t producer = {.logger = &logger, .sink = &test_sink, .first_sequence = 0U, .count = BINARY_LOGGER_QUEUE_CAPACITY};
    pthread_t producer_thread;

    InitTestSink(&test_sink);
    test_sink.block_open = true;
    RequireStatus(BinaryLoggerStart(&logger, &descriptor, &sink), BINARY_LOGGER_STATUS_OK, "could not start logger");
    Require(WaitForCondition(IsOpenEntered, &test_sink), "consumer did not enter blocked sink open");
    Require(pthread_create(&producer_thread, NULL, PublishSamples, &producer) == 0, "could not start producer");
    Require(WaitForCondition(IsProducerFinished, &producer), "producer waited for queue space or sink I/O");
    Require(producer.accepted == BINARY_LOGGER_QUEUE_CAPACITY - 1U, "producer did not publish exactly the bounded queue capacity");
    Require(pthread_mutex_lock(&test_sink.mutex) == 0, "could not lock test sink");
    test_sink.release_open = true;
    Require(pthread_cond_broadcast(&test_sink.condition) == 0, "could not release sink open");
    Require(pthread_mutex_unlock(&test_sink.mutex) == 0, "could not unlock test sink");
    Require(pthread_join(producer_thread, NULL) == 0, "could not join producer");
    RequireStatus(BinaryLoggerJoin(&logger, &result), BINARY_LOGGER_STATUS_OK, "could not join logger");
    Require(result.accepted == BINARY_LOGGER_QUEUE_CAPACITY - 1U && result.persisted == result.accepted, "accepted samples were not persisted after blocked consumer release");
    Require(result.discarded == 1U && result.incomplete, "saturation was not counted as incomplete logging");
    Require(test_sink.producer_callback_calls == 0U, "producer invoked a sink callback");
    DestroyTestSink(&test_sink);
}

static void WaitForRecords(test_sink_t *sink, size_t expected_count) {
    struct timespec deadline;

    Require(clock_gettime(CLOCK_REALTIME, &deadline) == 0, "could not read real-time clock");
    deadline.tv_sec += 2;
    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock test sink");
    while (sink->record_calls < expected_count) {
        const int wait_status = pthread_cond_timedwait(&sink->condition, &sink->mutex, &deadline);

        Require(wait_status == 0, wait_status == ETIMEDOUT ? "consumer did not persist expected records" : "could not wait for consumer records");
    }
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock test sink");
}

static void *PublishWrapSamples(void *argument) {
    producer_context_t *producer = argument;
    size_t published = 0U;
    const size_t batch_size = BINARY_LOGGER_QUEUE_CAPACITY / 2U;

    SetProducerThread(producer->sink);
    while (published < producer->count) {
        const size_t batch_end = published + batch_size < producer->count ? published + batch_size : producer->count;

        while (published < batch_end) {
            const log_sample_t sample = MakeSample(producer->first_sequence + (uint64_t)published);

            Require(BinaryLoggerPublish(producer->logger, &sample), "controlled concurrent producer saturated queue");
            ++producer->accepted;
            ++published;
        }
        WaitForRecords(producer->sink, published);
    }
    BinaryLoggerProducerDone(producer->logger);
    atomic_store_explicit(&producer->finished, true, memory_order_release);
    return NULL;
}

/* @spec:AC-021 */
static void TestConcurrentWrapKeepsOrderAndUniqueness(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t test_sink;
    const binary_logger_sink_t sink = MakeTestSink(&test_sink);
    binary_logger_t logger;
    binary_logger_result_t result;
    const size_t sample_count = BINARY_LOGGER_QUEUE_CAPACITY * 3U + 17U;
    producer_context_t producer = {.logger = &logger, .sink = &test_sink, .first_sequence = 100U, .count = sample_count};
    pthread_t producer_thread;

    InitTestSink(&test_sink);
    RequireStatus(BinaryLoggerStart(&logger, &descriptor, &sink), BINARY_LOGGER_STATUS_OK, "could not start logger");
    Require(pthread_create(&producer_thread, NULL, PublishWrapSamples, &producer) == 0, "could not start producer");
    Require(pthread_join(producer_thread, NULL) == 0, "could not join producer");
    RequireStatus(BinaryLoggerJoin(&logger, &result), BINARY_LOGGER_STATUS_OK, "could not join logger");
    Require(result.accepted == sample_count && result.persisted == sample_count && result.discarded == 0U && !result.incomplete,
            "concurrent wrap did not preserve all samples without drops");
    Require(test_sink.record_calls == sample_count, "concurrent wrap did not write every sample exactly once");
    for (size_t index = 0U; index < sample_count; ++index) {
        Require(test_sink.sequences[index] == producer.first_sequence + (uint64_t)index, "concurrent wrap changed record order or uniqueness");
    }
    Require(test_sink.producer_callback_calls == 0U, "producer invoked a sink callback during concurrent wrap");
    DestroyTestSink(&test_sink);
}

static void *PublishAfterEmptyPoll(void *argument) {
    producer_context_t *producer = argument;
    const log_sample_t first = MakeSample(producer->first_sequence);
    const log_sample_t second = MakeSample(producer->first_sequence + 1U);
    uint64_t empty_polls;

    SetProducerThread(producer->sink);
    Require(BinaryLoggerPublish(producer->logger, &first), "could not publish first interleaving sample");
    producer->accepted = 1U;
    WaitForRecords(producer->sink, 1U);
    empty_polls = BinaryLoggerConsumerEmptyPolls(producer->logger);
    while (BinaryLoggerConsumerEmptyPolls(producer->logger) <= empty_polls) sched_yield();
    Require(BinaryLoggerPublish(producer->logger, &second), "could not publish second interleaving sample");
    ++producer->accepted;
    BinaryLoggerProducerDone(producer->logger);
    atomic_store_explicit(&producer->finished, true, memory_order_release);
    return NULL;
}

/* @spec:AC-019 */
static void TestProducerDoneAfterEmptyPollDrainsLastSample(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t test_sink;
    const binary_logger_sink_t sink = MakeTestSink(&test_sink);
    binary_logger_t logger;
    binary_logger_result_t result;
    producer_context_t producer = {.logger = &logger, .sink = &test_sink, .first_sequence = 700U, .count = 2U};
    pthread_t producer_thread;

    InitTestSink(&test_sink);
    RequireStatus(BinaryLoggerStart(&logger, &descriptor, &sink), BINARY_LOGGER_STATUS_OK, "could not start logger");
    Require(pthread_create(&producer_thread, NULL, PublishAfterEmptyPoll, &producer) == 0, "could not start producer");
    Require(pthread_join(producer_thread, NULL) == 0, "could not join producer");
    RequireStatus(BinaryLoggerJoin(&logger, &result), BINARY_LOGGER_STATUS_OK, "could not join logger");
    Require(result.accepted == 2U && result.persisted == 2U && result.discarded == 0U, "producer_done interleaving lost an accepted sample");
    Require(result.has_last_persisted_sequence && result.last_persisted_sequence == 701U, "last persisted sequence was not retained");
    Require(test_sink.record_calls == 2U && test_sink.sequences[0] == 700U && test_sink.sequences[1] == 701U, "interleaving changed record order");
    Require(test_sink.flush_calls == 1U && test_sink.close_calls == 1U, "normal finalization did not flush and close once");
    DestroyTestSink(&test_sink);
}

static void TestInjectedFailure(binary_logger_stage_t failure_stage) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t test_sink;
    const binary_logger_sink_t sink = MakeTestSink(&test_sink);
    binary_logger_t logger;
    binary_logger_result_t result;
    producer_context_t producer = {.logger = &logger, .sink = &test_sink, .first_sequence = 800U, .count = 3U};
    pthread_t producer_thread;

    InitTestSink(&test_sink);
    test_sink.failure_stage = failure_stage;
    RequireStatus(BinaryLoggerStart(&logger, &descriptor, &sink), BINARY_LOGGER_STATUS_OK, "could not start logger with failing sink");
    Require(pthread_create(&producer_thread, NULL, PublishSamples, &producer) == 0, "could not start failing-sink producer");
    Require(pthread_join(producer_thread, NULL) == 0, "could not join failing-sink producer");
    RequireStatus(BinaryLoggerJoin(&logger, &result), BINARY_LOGGER_STATUS_OK, "could not join failing logger");
    Require(producer.accepted == 3U && result.accepted == 3U, "sink failure stopped the producer");
    Require(result.incomplete && result.first_error_stage == failure_stage && result.first_error == BINARY_LOGGER_STATUS_SINK_FAILURE,
            "logger did not preserve the first structured sink error");
    Require(strcmp(result.error_message, "deterministic sink failure") == 0, "logger did not preserve bounded sink diagnostic");
    Require(test_sink.producer_callback_calls == 0U, "producer invoked sink callback after failure");
    if (failure_stage == BINARY_LOGGER_STAGE_OPEN) {
        Require(test_sink.header_calls == 0U && test_sink.record_calls == 0U && test_sink.flush_calls == 0U && test_sink.close_calls == 0U,
                "open failure performed an inapplicable sink operation");
        Require(result.persisted == 0U && result.discarded == 3U, "open failure did not account for discarded samples");
    } else if (failure_stage == BINARY_LOGGER_STAGE_HEADER || failure_stage == BINARY_LOGGER_STAGE_RECORD) {
        Require(test_sink.flush_calls == 1U && test_sink.close_calls == 1U, "opened failing sink was not finalized once");
        Require(result.persisted == 0U && result.discarded == 3U, "failed sink did not discard all unpersisted samples");
    } else {
        Require(result.persisted == 3U && result.discarded == 0U && test_sink.flush_calls == 1U && test_sink.close_calls == 1U,
                "finalization failure changed persisted sample accounting");
    }
    DestroyTestSink(&test_sink);
}

/* @spec:AC-020 @spec:AC-026 */
static void TestSinkFailuresRemainStructured(void) {
    TestInjectedFailure(BINARY_LOGGER_STAGE_OPEN);
    TestInjectedFailure(BINARY_LOGGER_STAGE_HEADER);
    TestInjectedFailure(BINARY_LOGGER_STAGE_RECORD);
    TestInjectedFailure(BINARY_LOGGER_STAGE_FLUSH);
    TestInjectedFailure(BINARY_LOGGER_STAGE_CLOSE);
}

/* @spec:AC-018 @spec:AC-019 */
static void TestFileSinkUsesLogFormatCodec(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    binary_logger_file_sink_t file_sink;
    binary_logger_sink_t sink;
    binary_logger_t logger;
    binary_logger_result_t result;
    char path[] = "/tmp/microhil-binary-logger-XXXXXX";
    log_descriptor_t decoded_descriptor;
    log_sample_t decoded_sample;
    FILE *file;
    int descriptor_fd;

    descriptor_fd = mkstemp(path);
    Require(descriptor_fd >= 0, "could not reserve temporary log path");
    Require(close(descriptor_fd) == 0, "could not close temporary log path");
    BinaryLoggerFileSinkInit(&file_sink, path, &sink);
    RequireStatus(BinaryLoggerStart(&logger, &descriptor, &sink), BINARY_LOGGER_STATUS_OK, "could not start file logger");
    for (uint64_t sequence = 0U; sequence < 3U; ++sequence) {
        const log_sample_t sample = MakeSample(sequence);

        Require(BinaryLoggerPublish(&logger, &sample), "file logger rejected sample without saturation");
    }
    BinaryLoggerProducerDone(&logger);
    {
        const log_sample_t late_sample = MakeSample(99U);

        Require(!BinaryLoggerPublish(&logger, &late_sample), "file logger accepted a sample after producer_done");
    }
    RequireStatus(BinaryLoggerJoin(&logger, &result), BINARY_LOGGER_STATUS_OK, "could not join file logger");
    Require(!result.incomplete && result.persisted == 3U && result.last_persisted_sequence == 2U, "file logger result is incomplete");
    file = fopen(path, "rb");
    Require(file != NULL, "could not reopen binary log");
    Require(LogFormatReadHeader(file, &decoded_descriptor) == LOG_FORMAT_STATUS_OK, "file sink did not serialize codec header");
    Require(decoded_descriptor.output_count == 1U && decoded_descriptor.outputs[0].type == LOG_VALUE_REAL, "file sink changed codec descriptor");
    for (uint64_t sequence = 0U; sequence < 3U; ++sequence) {
        Require(LogFormatReadRecord(file, &decoded_descriptor, &decoded_sample) == LOG_FORMAT_STATUS_OK, "file sink did not serialize codec record");
        Require(decoded_sample.sequence == sequence && decoded_sample.values[0].real_value == (double)sequence, "file sink changed codec record contents");
    }
    Require(LogFormatReadRecord(file, &decoded_descriptor, &decoded_sample) == LOG_FORMAT_STATUS_END_OF_FILE, "file sink wrote unexpected record");
    Require(fclose(file) == 0, "could not close binary log");
    Require(unlink(path) == 0, "could not remove temporary binary log");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file|saturation|wrap|interleaving|failures>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "file") == 0) {
        TestFileSinkUsesLogFormatCodec();
    } else if (strcmp(argv[1], "saturation") == 0) {
        TestSaturationDoesNotBlockProducer();
    } else if (strcmp(argv[1], "wrap") == 0) {
        TestConcurrentWrapKeepsOrderAndUniqueness();
    } else if (strcmp(argv[1], "interleaving") == 0) {
        TestProducerDoneAfterEmptyPollDrainsLastSample();
    } else if (strcmp(argv[1], "failures") == 0) {
        TestSinkFailuresRemainStructured();
    } else {
        fprintf(stderr, "unknown test: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
