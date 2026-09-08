#include "run_logging.h"

#include "sha256.h"

#include <math.h>
#include <string.h>

static bool HashIsZero(const uint8_t hash[LOG_FORMAT_SHA256_SIZE]) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < LOG_FORMAT_SHA256_SIZE; ++index) combined |= hash[index];
    return combined == 0U;
}

static bool MapType(NumericType type, log_value_type_t *mapped) {
    switch (type) {
        case NUMERIC_REAL: *mapped = LOG_VALUE_REAL; return true;
        case NUMERIC_INTEGER: *mapped = LOG_VALUE_INTEGER; return true;
        case NUMERIC_BOOLEAN: *mapped = LOG_VALUE_BOOLEAN; return true;
        case NUMERIC_ENUMERATION: *mapped = LOG_VALUE_ENUMERATION; return true;
        default: return false;
    }
}

run_logging_status_t RunLoggingBuildDescriptor(const char *fmu_path, const OutputVariable *outputs, size_t output_count, double step_size_s,
                                               log_descriptor_t *descriptor) {
    sha256_status_t hash_status;

    if (!fmu_path || !outputs || !descriptor || output_count == 0U || output_count > LOG_FORMAT_MAX_OUTPUTS || !isfinite(step_size_s) || step_size_s <= 0.0) {
        return RUN_LOGGING_STATUS_DESCRIPTOR;
    }
    *descriptor = (log_descriptor_t){.t_start_s = 0.0, .step_size_s = step_size_s, .output_count = (uint32_t)output_count};
    hash_status = Sha256File(fmu_path, descriptor->fmu_sha256);
    if (hash_status != SHA256_STATUS_OK || HashIsZero(descriptor->fmu_sha256)) return RUN_LOGGING_STATUS_SHA256;
    for (size_t index = 0U; index < output_count; ++index) {
        log_value_type_t type;
        if (!MapType(outputs[index].type, &type) || outputs[index].name[0] == '\0') return RUN_LOGGING_STATUS_DESCRIPTOR;
        descriptor->outputs[index] = (log_output_t){.xml_index = outputs[index].xml_index, .value_reference = outputs[index].value_reference, .type = type};
        strncpy(descriptor->outputs[index].name, outputs[index].name, sizeof(descriptor->outputs[index].name) - 1U);
    }
    return LogFormatHeaderSize(descriptor, &(uint32_t){0}) == LOG_FORMAT_STATUS_OK ? RUN_LOGGING_STATUS_OK : RUN_LOGGING_STATUS_DESCRIPTOR;
}

void RunLoggingInit(run_logging_t *logging, bool enabled) {
    if (!logging) return;
    *logging = (run_logging_t){.result = {.enabled = enabled, .status = enabled ? RUN_LOGGING_STATUS_OK : RUN_LOGGING_STATUS_DISABLED}};
}

static void RememberStartFailure(run_logging_t *logging, binary_logger_status_t status) {
    logging->result.status = RUN_LOGGING_STATUS_START;
    logging->result.logger.incomplete = true;
    logging->result.logger.first_error_stage = BINARY_LOGGER_STAGE_OPEN;
    logging->result.logger.first_error = status;
    strncpy(logging->result.logger.error_message, BinaryLoggerStatusString(status), sizeof(logging->result.logger.error_message) - 1U);
}

run_logging_status_t RunLoggingStart(run_logging_t *logging, const log_descriptor_t *descriptor, const binary_logger_sink_t *sink) {
    binary_logger_status_t status;
    if (!logging) return RUN_LOGGING_STATUS_DESCRIPTOR;
    if (!logging->result.enabled) return RUN_LOGGING_STATUS_DISABLED;
    if (!descriptor || LogFormatHeaderSize(descriptor, &(uint32_t){0}) != LOG_FORMAT_STATUS_OK || HashIsZero(descriptor->fmu_sha256)) {
        logging->result.status = RUN_LOGGING_STATUS_DESCRIPTOR;
        logging->result.logger.incomplete = true;
        logging->result.logger.first_error_stage = BINARY_LOGGER_STAGE_HEADER;
        logging->result.logger.first_error = BINARY_LOGGER_STATUS_INVALID_ARGUMENT;
        strncpy(logging->result.logger.error_message, "invalid log descriptor", sizeof(logging->result.logger.error_message) - 1U);
        return logging->result.status;
    }
    logging->descriptor = *descriptor;
    if (!sink) {
        RememberStartFailure(logging, BINARY_LOGGER_STATUS_INVALID_ARGUMENT);
        return logging->result.status;
    }
    logging->sink = *sink;
    status = BinaryLoggerStart(&logging->logger, &logging->descriptor, &logging->sink);
    if (status != BINARY_LOGGER_STATUS_OK) {
        RememberStartFailure(logging, status);
        return logging->result.status;
    }
    logging->result.started = true;
    return RUN_LOGGING_STATUS_OK;
}

run_logging_status_t RunLoggingStartFile(run_logging_t *logging, const log_descriptor_t *descriptor, const char *path) {
    if (!logging) return RUN_LOGGING_STATUS_DESCRIPTOR;
    if (!logging->result.enabled) return RUN_LOGGING_STATUS_DISABLED;
    if (!path || path[0] == '\0') {
        RememberStartFailure(logging, BINARY_LOGGER_STATUS_INVALID_ARGUMENT);
        return logging->result.status;
    }
    BinaryLoggerFileSinkInit(&logging->file_sink, path, &logging->sink);
    return RunLoggingStart(logging, descriptor, &logging->sink);
}

bool RunLoggingPublish(run_logging_t *logging, const log_sample_t *sample) {
    return logging && logging->result.started ? BinaryLoggerPublish(&logging->logger, sample) : false;
}

void RunLoggingProducerDone(run_logging_t *logging) {
    if (logging && logging->result.started) BinaryLoggerProducerDone(&logging->logger);
}

void RunLoggingFinish(run_logging_t *logging) {
    if (!logging || !logging->result.started) return;
    BinaryLoggerProducerDone(&logging->logger);
    if (BinaryLoggerJoin(&logging->logger, &logging->result.logger) != BINARY_LOGGER_STATUS_OK) {
        logging->result.status = RUN_LOGGING_STATUS_JOIN;
        logging->result.logger.incomplete = true;
    } else if (logging->result.logger.incomplete) {
        logging->result.status = RUN_LOGGING_STATUS_INCOMPLETE;
    }
    logging->result.started = false;
}

const run_logging_result_t *RunLoggingResult(const run_logging_t *logging) {
    return logging ? &logging->result : NULL;
}

const char *RunLoggingStatusString(run_logging_status_t status) {
    switch (status) {
        case RUN_LOGGING_STATUS_OK: return "ok";
        case RUN_LOGGING_STATUS_DISABLED: return "disabled";
        case RUN_LOGGING_STATUS_DESCRIPTOR: return "invalid descriptor";
        case RUN_LOGGING_STATUS_SHA256: return "SHA-256 failure";
        case RUN_LOGGING_STATUS_START: return "logger start failure";
        case RUN_LOGGING_STATUS_JOIN: return "logger join failure";
        case RUN_LOGGING_STATUS_INCOMPLETE: return "incomplete log";
        default: return "unknown logging status";
    }
}
