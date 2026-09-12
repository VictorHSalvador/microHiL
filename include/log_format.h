#ifndef LOG_FORMAT_H
#define LOG_FORMAT_H

/* Contrato público do módulo log format. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define LOG_FORMAT_SHA256_SIZE 32U
#define LOG_FORMAT_MAX_OUTPUTS 64U
#define LOG_FORMAT_MAX_NAME_BYTES 255U
#define LOG_FORMAT_QUALITY_BYTES ((LOG_FORMAT_MAX_OUTPUTS + 7U) / 8U)

typedef enum {
    LOG_VALUE_REAL = 0,
    LOG_VALUE_INTEGER = 1,
    LOG_VALUE_BOOLEAN = 2,
    LOG_VALUE_ENUMERATION = 3
} log_value_type_t;

typedef enum {
    LOG_FORMAT_STATUS_OK = 0,
    LOG_FORMAT_STATUS_END_OF_FILE,
    LOG_FORMAT_STATUS_INVALID_ARGUMENT,
    LOG_FORMAT_STATUS_IO_ERROR,
    LOG_FORMAT_STATUS_TRUNCATED,
    LOG_FORMAT_STATUS_MAGIC,
    LOG_FORMAT_STATUS_VERSION,
    LOG_FORMAT_STATUS_HEADER_SIZE,
    LOG_FORMAT_STATUS_LIMIT,
    LOG_FORMAT_STATUS_OVERFLOW,
    LOG_FORMAT_STATUS_TYPE,
    LOG_FORMAT_STATUS_ORDER,
    LOG_FORMAT_STATUS_UTF8,
    LOG_FORMAT_STATUS_INCONSISTENT
} log_format_status_t;

typedef struct {
    uint32_t xml_index;
    uint32_t value_reference;
    log_value_type_t type;
    char name[LOG_FORMAT_MAX_NAME_BYTES + 1U];
} log_output_t;

typedef struct {
    uint8_t fmu_sha256[LOG_FORMAT_SHA256_SIZE];
    double t_start_s;
    double step_size_s;
    uint32_t output_count;
    log_output_t outputs[LOG_FORMAT_MAX_OUTPUTS];
} log_descriptor_t;

typedef union {
    double real_value;
    int64_t discrete_value;
    uint8_t boolean_value;
} log_value_t;

typedef struct {
    uint64_t sequence;
    double sim_time_s;
    uint8_t quality_bitmap[LOG_FORMAT_QUALITY_BYTES];
    log_value_t values[LOG_FORMAT_MAX_OUTPUTS];
} log_sample_t;

size_t LogFormatQualityBytes(uint32_t output_count);
log_format_status_t LogFormatHeaderSize(const log_descriptor_t *descriptor, uint32_t *header_size);
log_format_status_t LogFormatWriteHeader(FILE *file, const log_descriptor_t *descriptor);
log_format_status_t LogFormatReadHeader(FILE *file, log_descriptor_t *descriptor);
log_format_status_t LogFormatWriteRecord(FILE *file, const log_descriptor_t *descriptor, const log_sample_t *sample);
log_format_status_t LogFormatReadRecord(FILE *file, const log_descriptor_t *descriptor, log_sample_t *sample);
const char *LogFormatStatusString(log_format_status_t status);

#endif
