#ifndef RUN_LOGGING_H
#define RUN_LOGGING_H

/* Contrato público do módulo run logging. */

#include <stdbool.h>

#include "app_config.h"
#include "binary_logger.h"

typedef enum {
    RUN_LOGGING_STATUS_OK = 0,
    RUN_LOGGING_STATUS_DISABLED,
    RUN_LOGGING_STATUS_DESCRIPTOR,
    RUN_LOGGING_STATUS_SHA256,
    RUN_LOGGING_STATUS_START,
    RUN_LOGGING_STATUS_JOIN,
    RUN_LOGGING_STATUS_INCOMPLETE
} run_logging_status_t;

typedef struct {
    bool enabled;
    bool started;
    run_logging_status_t status;
    binary_logger_result_t logger;
} run_logging_result_t;

typedef struct {
    log_descriptor_t descriptor;
    binary_logger_t logger;
    binary_logger_file_sink_t file_sink;
    binary_logger_sink_t sink;
    run_logging_result_t result;
} run_logging_t;

run_logging_status_t RunLoggingBuildDescriptor(const char *fmu_path, const OutputVariable *outputs, size_t output_count, double step_size_s,
                                               log_descriptor_t *descriptor);
void RunLoggingInit(run_logging_t *logging, bool enabled);
run_logging_status_t RunLoggingStart(run_logging_t *logging, const log_descriptor_t *descriptor, const binary_logger_sink_t *sink);
run_logging_status_t RunLoggingStartFile(run_logging_t *logging, const log_descriptor_t *descriptor, const char *path);
bool RunLoggingPublish(run_logging_t *logging, const log_sample_t *sample);
void RunLoggingProducerDone(run_logging_t *logging);
void RunLoggingFinish(run_logging_t *logging);
const run_logging_result_t *RunLoggingResult(const run_logging_t *logging);
const char *RunLoggingStatusString(run_logging_status_t status);

#endif
