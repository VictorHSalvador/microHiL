#ifndef LOG_CONVERTER_H
#define LOG_CONVERTER_H

#include <stdbool.h>
#include <stdint.h>

#include "log_format.h"

#define LOG_CONVERTER_MESSAGE_SIZE 128U

typedef enum {
    LOG_CONVERTER_EXECUTION_RUNNING = 0,
    LOG_CONVERTER_EXECUTION_CLOSED
} log_converter_execution_state_t;

typedef enum {
    LOG_CONVERTER_STATUS_OK = 0,
    LOG_CONVERTER_STATUS_PARTIAL,
    LOG_CONVERTER_STATUS_INVALID_ARGUMENT,
    LOG_CONVERTER_STATUS_RUNNING,
    LOG_CONVERTER_STATUS_INPUT_OPEN,
    LOG_CONVERTER_STATUS_HEADER,
    LOG_CONVERTER_STATUS_SCHEMA_MISMATCH,
    LOG_CONVERTER_STATUS_TEMPORARY_OPEN,
    LOG_CONVERTER_STATUS_OUTPUT_WRITE,
    LOG_CONVERTER_STATUS_OUTPUT_CLOSE,
    LOG_CONVERTER_STATUS_RENAME,
    LOG_CONVERTER_STATUS_STRUCTURAL,
    LOG_CONVERTER_STATUS_OVERFLOW
} log_converter_status_t;

typedef enum {
    LOG_CONVERTER_FIELD_NONE = 0,
    LOG_CONVERTER_FIELD_SHA256,
    LOG_CONVERTER_FIELD_OUTPUT_COUNT,
    LOG_CONVERTER_FIELD_XML_INDEX,
    LOG_CONVERTER_FIELD_VALUE_REFERENCE,
    LOG_CONVERTER_FIELD_TYPE,
    LOG_CONVERTER_FIELD_NAME,
    LOG_CONVERTER_FIELD_SEQUENCE
} log_converter_field_t;

typedef struct {
    log_converter_status_t status;
    log_converter_field_t field;
    uint32_t output_index;
    uint64_t record_index;
    uint64_t exported_records;
    uint64_t missing_sequences;
    bool truncated;
    char message[LOG_CONVERTER_MESSAGE_SIZE];
} log_converter_result_t;

log_converter_status_t LogConverterConvert(log_converter_execution_state_t execution_state, const char *binary_path, const char *csv_path,
                                           const log_descriptor_t *expected_descriptor, log_converter_result_t *result);
const char *LogConverterStatusString(log_converter_status_t status);
const char *LogConverterFieldString(log_converter_field_t field);

#endif
