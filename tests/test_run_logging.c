#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "run_logging.h"
#include "run_result.h"

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool block_open;
    bool open_entered;
    bool released;
    bool fail_record;
    uint64_t sequences[BINARY_LOGGER_QUEUE_CAPACITY];
    log_sample_t samples[4];
    size_t record_count;
    size_t flush_count;
    size_t close_count;
} test_sink_t;

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void InitSink(test_sink_t *sink) {
    *sink = (test_sink_t){0};
    Require(pthread_mutex_init(&sink->mutex, NULL) == 0, "could not initialize sink mutex");
    Require(pthread_cond_init(&sink->condition, NULL) == 0, "could not initialize sink condition");
}

static void DestroySink(test_sink_t *sink) {
    Require(pthread_cond_destroy(&sink->condition) == 0, "could not destroy sink condition");
    Require(pthread_mutex_destroy(&sink->mutex) == 0, "could not destroy sink mutex");
}

static binary_logger_status_t SinkOpen(void *context) {
    test_sink_t *sink = context;
    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock sink");
    sink->open_entered = true;
    Require(pthread_cond_broadcast(&sink->condition) == 0, "could not signal sink open");
    while (sink->block_open && !sink->released) Require(pthread_cond_wait(&sink->condition, &sink->mutex) == 0, "could not wait in sink open");
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock sink");
    return BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t SinkHeader(void *context, const log_descriptor_t *descriptor) {
    (void)context;
    (void)descriptor;
    return BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t SinkRecord(void *context, const log_descriptor_t *descriptor, const log_sample_t *sample) {
    test_sink_t *sink = context;
    (void)descriptor;
    Require(pthread_mutex_lock(&sink->mutex) == 0, "could not lock sink record");
    if (sink->record_count < sizeof(sink->samples) / sizeof(sink->samples[0])) sink->samples[sink->record_count] = *sample;
    if (sink->record_count < sizeof(sink->sequences) / sizeof(sink->sequences[0])) sink->sequences[sink->record_count] = sample->sequence;
    ++sink->record_count;
    Require(pthread_mutex_unlock(&sink->mutex) == 0, "could not unlock sink record");
    return sink->fail_record ? BINARY_LOGGER_STATUS_SINK_FAILURE : BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t SinkFlush(void *context) {
    test_sink_t *sink = context;
    ++sink->flush_count;
    return BINARY_LOGGER_STATUS_OK;
}

static binary_logger_status_t SinkClose(void *context) {
    test_sink_t *sink = context;
    ++sink->close_count;
    return BINARY_LOGGER_STATUS_OK;
}

static const char *SinkMessage(void *context, binary_logger_status_t status) {
    (void)context;
    return status == BINARY_LOGGER_STATUS_SINK_FAILURE ? "injected record failure" : BinaryLoggerStatusString(status);
}

static binary_logger_sink_t MakeSink(test_sink_t *sink) {
    return (binary_logger_sink_t){.context = sink, .open = SinkOpen, .write_header = SinkHeader, .write_record = SinkRecord,
                                  .flush = SinkFlush, .close = SinkClose, .message = SinkMessage};
}

static void MakeFmuPath(char path[32]) {
    int fd;
    snprintf(path, 32U, "/tmp/microhil-fmu-XXXXXX");
    fd = mkstemp(path);
    Require(fd >= 0, "could not create FMU hash fixture");
    Require(write(fd, "abc", 3U) == 3, "could not write FMU hash fixture");
    Require(close(fd) == 0, "could not close FMU hash fixture");
}

static AppConfig MakeConfig(void) {
    AppConfig config;
    app_config_set_defaults(&config);
    config.output_count = 3U;
    config.outputs[0] = (OutputVariable){.xml_index = 9U, .value_reference = 17U, .type = NUMERIC_BOOLEAN, .name = "enabled"};
    config.outputs[1] = (OutputVariable){.xml_index = 2U, .value_reference = 17U, .type = NUMERIC_INTEGER, .name = "count"};
    config.outputs[2] = (OutputVariable){.xml_index = 5U, .value_reference = 19U, .type = NUMERIC_REAL, .name = "value"};
    return config;
}

static log_descriptor_t MakeDescriptor(void) {
    AppConfig config = MakeConfig();
    log_descriptor_t descriptor;
    char path[32];
    MakeFmuPath(path);
    Require(app_config_normalize_outputs(&config) == 0, "could not canonicalize outputs");
    Require(RunLoggingBuildDescriptor(path, config.outputs, config.output_count, config.step_size_s, &descriptor) == 0, "could not build descriptor");
    Require(unlink(path) == 0, "could not remove FMU hash fixture");
    return descriptor;
}

static log_sample_t MakeSample(uint64_t sequence) {
    log_sample_t sample = {.sequence = sequence, .sim_time_s = (double)(sequence + 1U) * 0.02};
    sample.values[0].discrete_value = -7;
    sample.values[1].real_value = 3.25;
    sample.values[2].boolean_value = 1U;
    return sample;
}

/* @spec:AC-018 @spec:AC-019 */
static void TestEnabledTypedDrain(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t sink_context;
    const binary_logger_sink_t sink = MakeSink(&sink_context);
    run_logging_t logging;
    const log_sample_t first = MakeSample(0U);
    const log_sample_t second = MakeSample(1U);

    InitSink(&sink_context);
    RunLoggingInit(&logging, true);
    Require(RunLoggingStart(&logging, &descriptor, &sink) == RUN_LOGGING_STATUS_OK, "could not start enabled logging");
    Require(RunLoggingPublish(&logging, &first) && RunLoggingPublish(&logging, &second), "enabled logging rejected a sample");
    RunLoggingProducerDone(&logging);
    RunLoggingFinish(&logging);
    Require(logging.result.status == RUN_LOGGING_STATUS_OK && !logging.result.logger.incomplete, "complete logging did not retain a complete status");
    Require(logging.result.logger.accepted == 2U && logging.result.logger.persisted == 2U && logging.result.logger.discarded == 0U,
            "enabled logging did not drain accepted samples");
    Require(sink_context.record_count == 2U && sink_context.flush_count == 1U && sink_context.close_count == 1U, "logger lifecycle did not flush and close once");
    Require(sink_context.samples[0].values[0].discrete_value == -7 && sink_context.samples[0].values[1].real_value == 3.25 &&
            sink_context.samples[0].values[2].boolean_value == 1U, "typed sample changed before the sink");
    DestroySink(&sink_context);
}

/* @spec:AC-025 */
static void TestDisabledDoesNotPublish(void) {
    run_logging_t logging;
    const log_sample_t sample = MakeSample(0U);

    RunLoggingInit(&logging, false);
    Require(!RunLoggingPublish(&logging, &sample), "disabled logger accepted a sample");
    RunLoggingFinish(&logging);
    Require(logging.result.status == RUN_LOGGING_STATUS_DISABLED && !logging.result.logger.incomplete, "disabled state is not structured");
}

/* @spec:AC-018 @spec:AC-025 */
static void TestSaturationIsSeparate(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t sink_context;
    const binary_logger_sink_t sink = MakeSink(&sink_context);
    run_logging_t logging;
    size_t accepted = 0U;

    InitSink(&sink_context);
    sink_context.block_open = true;
    RunLoggingInit(&logging, true);
    Require(RunLoggingStart(&logging, &descriptor, &sink) == RUN_LOGGING_STATUS_OK, "could not start saturation logger");
    Require(pthread_mutex_lock(&sink_context.mutex) == 0, "could not wait for blocked open");
    while (!sink_context.open_entered) Require(pthread_cond_wait(&sink_context.condition, &sink_context.mutex) == 0, "could not wait for sink open");
    Require(pthread_mutex_unlock(&sink_context.mutex) == 0, "could not unlock blocked open");
    for (uint64_t sequence = 0U; sequence < BINARY_LOGGER_QUEUE_CAPACITY; ++sequence) {
        const log_sample_t sample = MakeSample(sequence);
        if (RunLoggingPublish(&logging, &sample)) ++accepted;
    }
    Require(accepted == BINARY_LOGGER_QUEUE_CAPACITY - 1U, "producer waited or exceeded the bounded queue");
    Require(pthread_mutex_lock(&sink_context.mutex) == 0, "could not release sink");
    sink_context.released = true;
    Require(pthread_cond_broadcast(&sink_context.condition) == 0, "could not signal sink release");
    Require(pthread_mutex_unlock(&sink_context.mutex) == 0, "could not unlock released sink");
    RunLoggingFinish(&logging);
    Require(logging.result.status == RUN_LOGGING_STATUS_INCOMPLETE && logging.result.logger.discarded == 1U && logging.result.logger.incomplete,
            "saturation was not visible in logging result");
    DestroySink(&sink_context);
}

/* @spec:AC-020 @spec:AC-025 @spec:AC-026 */
static void TestFailureIsStructuredAndSeparate(void) {
    const log_descriptor_t descriptor = MakeDescriptor();
    test_sink_t sink_context;
    const binary_logger_sink_t sink = MakeSink(&sink_context);
    run_logging_t logging;

    InitSink(&sink_context);
    sink_context.fail_record = true;
    RunLoggingInit(&logging, true);
    Require(RunLoggingStart(&logging, &descriptor, &sink) == RUN_LOGGING_STATUS_OK, "could not start failing logger");
    {
        const log_sample_t sample = MakeSample(4U);
        Require(RunLoggingPublish(&logging, &sample), "failing logger stopped producer publication");
    }
    RunLoggingFinish(&logging);
    Require(logging.result.status == RUN_LOGGING_STATUS_INCOMPLETE && logging.result.logger.incomplete && logging.result.logger.first_error_stage == BINARY_LOGGER_STAGE_RECORD &&
            strcmp(logging.result.logger.error_message, "injected record failure") == 0, "sink failure is not available as a structured result");
    DestroySink(&sink_context);
}

/* @spec:AC-025 */
static void TestRunResultKeepsSimulationIndependent(void) {
    const simulation_run_result_t simulation = {
        .state = SIMULATION_RUN_FINISHED,
        .code = 0,
        .stage = "complete",
        .message = "fixture result",
        .stats = {.completed_steps = 125U, .deadline_misses = 3U, .max_computation_s = 0.004, .max_lateness_s = 0.002, .sched_fifo_active = true}
    };
    run_logging_result_t logging = {.enabled = false, .status = RUN_LOGGING_STATUS_DISABLED};
    run_result_t first;
    run_result_t second;

    RunResultAggregate(&first, &simulation, &logging);
    logging = (run_logging_result_t){.enabled = true, .status = RUN_LOGGING_STATUS_INCOMPLETE,
                                     .logger = {.accepted = 120U, .persisted = 119U, .discarded = 1U, .incomplete = true}};
    RunResultAggregate(&second, &simulation, &logging);
    Require(memcmp(&first.simulation, &second.simulation, sizeof(first.simulation)) == 0, "logging result altered simulation state or metrics");
    Require(second.logging.status == RUN_LOGGING_STATUS_INCOMPLETE && second.logging.logger.discarded == 1U,
            "aggregated logging result lost its independent status");
}

/* @spec:AC-019 @spec:AC-025 */
static void TestDescriptorOrderAndHash(void) {
    AppConfig config = MakeConfig();
    log_descriptor_t descriptor;
    char path[32];
    const uint8_t abc_hash[LOG_FORMAT_SHA256_SIZE] = {0xbaU, 0x78U, 0x16U, 0xbfU, 0x8fU, 0x01U, 0xcfU, 0xeaU, 0x41U, 0x41U, 0x40U, 0xdeU, 0x5dU, 0xaeU, 0x22U, 0x23U, 0xb0U, 0x03U, 0x61U, 0xa3U, 0x96U, 0x17U, 0x7aU, 0x9cU, 0xb4U, 0x10U, 0xffU, 0x61U, 0xf2U, 0x00U, 0x15U, 0xadU};

    MakeFmuPath(path);
    Require(app_config_normalize_outputs(&config) == 0, "could not normalize unordered selection");
    Require(config.outputs[0].xml_index == 2U && config.outputs[1].xml_index == 5U && config.outputs[2].xml_index == 9U, "selection was not ordered by XML index");
    Require(RunLoggingBuildDescriptor(path, config.outputs, config.output_count, config.step_size_s, &descriptor) == 0, "could not build canonical descriptor");
    Require(memcmp(descriptor.fmu_sha256, abc_hash, sizeof(abc_hash)) == 0, "SHA-256 does not match the known abc vector");
    Require(descriptor.outputs[0].xml_index == 2U && descriptor.outputs[0].value_reference == 17U && descriptor.outputs[1].xml_index == 5U &&
            descriptor.outputs[2].xml_index == 9U, "descriptor order diverged from normalized FMU read order");
    Require(unlink(path) == 0, "could not remove descriptor fixture");
    {
        run_logging_t logging;
        RunLoggingInit(&logging, true);
        Require(RunLoggingStartFile(&logging, &descriptor, "") == RUN_LOGGING_STATUS_START, "empty path did not produce a structured start failure");
        Require(logging.result.logger.incomplete && logging.result.logger.first_error_stage == BINARY_LOGGER_STAGE_OPEN,
                "empty path failure did not identify the open stage");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) return EXIT_FAILURE;
    if (strcmp(argv[1], "enabled") == 0) TestEnabledTypedDrain();
    else if (strcmp(argv[1], "disabled") == 0) TestDisabledDoesNotPublish();
    else if (strcmp(argv[1], "saturation") == 0) TestSaturationIsSeparate();
    else if (strcmp(argv[1], "failure") == 0) TestFailureIsStructuredAndSeparate();
    else if (strcmp(argv[1], "descriptor") == 0) TestDescriptorOrderAndHash();
    else if (strcmp(argv[1], "aggregate") == 0) TestRunResultKeepsSimulationIndependent();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
