/* Implementação do módulo log converter. */
#define _POSIX_C_SOURCE 200809L

#include "log_converter.h"

#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_CONVERTER_TEMPORARY_SUFFIX ".microhil-tmp.XXXXXX"

static void CopyMessage(log_converter_result_t *result, const char *message) {
    size_t index = 0U;

    if (!result) return;
    if (message) {
        while (index + 1U < sizeof(result->message) && message[index] != '\0') {
            result->message[index] = message[index];
            ++index;
        }
    }
    result->message[index] = '\0';
}

static void SetResult(log_converter_result_t *result, log_converter_status_t status, log_converter_field_t field, uint32_t output_index,
                      uint64_t record_index, const char *message) {
    if (!result) return;
    result->status = status;
    result->field = field;
    result->output_index = output_index;
    result->record_index = record_index;
    CopyMessage(result, message);
}

static bool IsInvalid(const log_sample_t *sample, uint32_t index) {
    return (sample->quality_bitmap[index / 8U] & (uint8_t)(UINT32_C(1) << (index % 8U))) != 0U;
}

static bool WriteText(FILE *file, const char *text) {
    return fputs(text, file) != EOF;
}

static bool WriteCharacter(FILE *file, int character) {
    return fputc(character, file) != EOF;
}

static bool WriteEscapedField(FILE *file, const char *text) {
    if (!WriteCharacter(file, '"')) return false;
    for (const unsigned char *cursor = (const unsigned char *)text; *cursor != '\0'; ++cursor) {
        if (*cursor == '"' && !WriteCharacter(file, '"')) return false;
        if (!WriteCharacter(file, *cursor)) return false;
    }
    return WriteCharacter(file, '"');
}

static bool WriteUnsigned64(FILE *file, uint64_t value) {
    char text[32];
    const int length = snprintf(text, sizeof(text), "%" PRIu64, value);

    return length >= 0 && (size_t)length < sizeof(text) && WriteText(file, text);
}

static bool WriteSigned64(FILE *file, int64_t value) {
    char text[32];
    const int length = snprintf(text, sizeof(text), "%" PRId64, value);

    return length >= 0 && (size_t)length < sizeof(text) && WriteText(file, text);
}

static bool WriteDouble(FILE *file, double value) {
    char text[64];
    locale_t c_locale;
    locale_t previous_locale;
    int length;

    if (isnan(value)) return WriteText(file, "NaN");
    c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!c_locale) return false;
    previous_locale = uselocale(c_locale);
    length = snprintf(text, sizeof(text), "%.17g", value);
    (void)uselocale(previous_locale);
    freelocale(c_locale);
    return length >= 0 && (size_t)length < sizeof(text) && WriteText(file, text);
}

static bool WriteNamedHeader(FILE *file, const char *name, const char *suffix) {
    char field[LOG_FORMAT_MAX_NAME_BYTES + 8U];
    const size_t name_length = strlen(name);
    const size_t suffix_length = strlen(suffix);

    if (name_length > sizeof(field) - suffix_length - 1U) return false;
    memcpy(field, name, name_length);
    memcpy(field + name_length, suffix, suffix_length + 1U);
    return WriteEscapedField(file, field);
}

static bool WriteHeader(FILE *file, const log_descriptor_t *descriptor) {
    if (!WriteText(file, "sequence,simulation_time_s")) return false;
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        if (!WriteCharacter(file, ',') || !WriteNamedHeader(file, descriptor->outputs[index].name, "_value") ||
            !WriteCharacter(file, ',') || !WriteNamedHeader(file, descriptor->outputs[index].name, "_valid")) {
            return false;
        }
    }
    return WriteCharacter(file, '\n');
}

static bool WriteSample(FILE *file, const log_descriptor_t *descriptor, const log_sample_t *sample) {
    if (!WriteUnsigned64(file, sample->sequence) || !WriteCharacter(file, ',') || !WriteDouble(file, sample->sim_time_s)) return false;
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        const log_value_type_t type = descriptor->outputs[index].type;
        const bool invalid = IsInvalid(sample, index);

        if (!WriteCharacter(file, ',')) return false;
        if (invalid) {
            if (type == LOG_VALUE_REAL) {
                if (!WriteText(file, "NaN")) return false;
            } else if (!WriteCharacter(file, '0')) {
                return false;
            }
        } else if (type == LOG_VALUE_REAL) {
            if (!WriteDouble(file, sample->values[index].real_value)) return false;
        } else if (type == LOG_VALUE_BOOLEAN) {
            if (!WriteCharacter(file, sample->values[index].boolean_value == 0U ? '0' : '1')) return false;
        } else if (!WriteSigned64(file, sample->values[index].discrete_value)) {
            return false;
        }
        if (!WriteCharacter(file, ',') || !WriteCharacter(file, invalid ? '0' : '1')) return false;
    }
    return WriteCharacter(file, '\n');
}

static bool BuildTemporaryPath(const char *csv_path, char **temporary_path) {
    const size_t csv_length = strlen(csv_path);
    const size_t suffix_length = sizeof(LOG_CONVERTER_TEMPORARY_SUFFIX);

    if (csv_length > SIZE_MAX - suffix_length) return false;
    *temporary_path = malloc(csv_length + suffix_length);
    if (!*temporary_path) return false;
    memcpy(*temporary_path, csv_path, csv_length);
    memcpy(*temporary_path + csv_length, LOG_CONVERTER_TEMPORARY_SUFFIX, suffix_length);
    return true;
}

static bool DescriptorsMatch(const log_descriptor_t *actual, const log_descriptor_t *expected, log_converter_result_t *result) {
    if (memcmp(actual->fmu_sha256, expected->fmu_sha256, LOG_FORMAT_SHA256_SIZE) != 0) {
        SetResult(result, LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, LOG_CONVERTER_FIELD_SHA256, UINT32_MAX, 0U, "FMU SHA-256 differs");
        return false;
    }
    if (actual->output_count != expected->output_count) {
        SetResult(result, LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, LOG_CONVERTER_FIELD_OUTPUT_COUNT, UINT32_MAX, 0U, "output count differs");
        return false;
    }
    for (uint32_t index = 0U; index < actual->output_count; ++index) {
        const log_output_t *actual_output = &actual->outputs[index];
        const log_output_t *expected_output = &expected->outputs[index];

        if (actual_output->xml_index != expected_output->xml_index) {
            SetResult(result, LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, LOG_CONVERTER_FIELD_XML_INDEX, index, 0U, "output XML index differs");
            return false;
        }
        if (actual_output->value_reference != expected_output->value_reference) {
            SetResult(result, LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, LOG_CONVERTER_FIELD_VALUE_REFERENCE, index, 0U, "output value reference differs");
            return false;
        }
        if (actual_output->type != expected_output->type) {
            SetResult(result, LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, LOG_CONVERTER_FIELD_TYPE, index, 0U, "output type differs");
            return false;
        }
        if (strcmp(actual_output->name, expected_output->name) != 0) {
            SetResult(result, LOG_CONVERTER_STATUS_SCHEMA_MISMATCH, LOG_CONVERTER_FIELD_NAME, index, 0U, "output name differs");
            return false;
        }
    }
    return true;
}

log_converter_status_t LogConverterConvert(log_converter_execution_state_t execution_state, const char *binary_path, const char *csv_path,
                                           const log_descriptor_t *expected_descriptor, log_converter_result_t *result) {
    log_descriptor_t actual_descriptor;
    log_format_status_t format_status;
    FILE *binary_file = NULL;
    FILE *csv_file = NULL;
    char *temporary_path = NULL;
    bool have_previous_sequence = false;
    uint64_t previous_sequence = 0U;
    uint64_t record_index = 0U;
    int temporary_fd;
    uint32_t expected_header_size;

    if (!result) return LOG_CONVERTER_STATUS_INVALID_ARGUMENT;
    *result = (log_converter_result_t){.status = LOG_CONVERTER_STATUS_INVALID_ARGUMENT, .output_index = UINT32_MAX};
    if (execution_state == LOG_CONVERTER_EXECUTION_RUNNING) {
        SetResult(result, LOG_CONVERTER_STATUS_RUNNING, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, "execution must be closed before conversion");
        return result->status;
    }
    if (execution_state != LOG_CONVERTER_EXECUTION_CLOSED || !binary_path || !csv_path || !expected_descriptor) {
        SetResult(result, LOG_CONVERTER_STATUS_INVALID_ARGUMENT, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, "invalid conversion arguments");
        return result->status;
    }
    format_status = LogFormatHeaderSize(expected_descriptor, &expected_header_size);
    if (format_status != LOG_FORMAT_STATUS_OK) {
        SetResult(result, LOG_CONVERTER_STATUS_INVALID_ARGUMENT, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, LogFormatStatusString(format_status));
        return result->status;
    }
    (void)expected_header_size;
    binary_file = fopen(binary_path, "rb");
    if (!binary_file) {
        SetResult(result, LOG_CONVERTER_STATUS_INPUT_OPEN, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, "could not open binary log");
        return result->status;
    }
    format_status = LogFormatReadHeader(binary_file, &actual_descriptor);
    if (format_status != LOG_FORMAT_STATUS_OK) {
        SetResult(result, LOG_CONVERTER_STATUS_HEADER, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, LogFormatStatusString(format_status));
        (void)fclose(binary_file);
        return result->status;
    }
    if (!DescriptorsMatch(&actual_descriptor, expected_descriptor, result)) {
        (void)fclose(binary_file);
        return result->status;
    }
    if (!BuildTemporaryPath(csv_path, &temporary_path)) {
        SetResult(result, LOG_CONVERTER_STATUS_OVERFLOW, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, "temporary path exceeds converter limits");
        (void)fclose(binary_file);
        return result->status;
    }
    temporary_fd = mkstemp(temporary_path);
    if (temporary_fd < 0 || !(csv_file = fdopen(temporary_fd, "w"))) {
        if (temporary_fd >= 0) (void)close(temporary_fd);
        (void)unlink(temporary_path);
        free(temporary_path);
        (void)fclose(binary_file);
        SetResult(result, LOG_CONVERTER_STATUS_TEMPORARY_OPEN, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, "could not create temporary CSV");
        return result->status;
    }
    if (!WriteHeader(csv_file, &actual_descriptor)) {
        SetResult(result, LOG_CONVERTER_STATUS_OUTPUT_WRITE, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, 0U, "could not write CSV header");
        goto cleanup_failure;
    }
    for (;;) {
        log_sample_t sample;

        format_status = LogFormatReadRecord(binary_file, &actual_descriptor, &sample);
        if (format_status == LOG_FORMAT_STATUS_END_OF_FILE) break;
        if (format_status == LOG_FORMAT_STATUS_TRUNCATED) {
            result->truncated = true;
            break;
        }
        if (format_status != LOG_FORMAT_STATUS_OK) {
            SetResult(result, LOG_CONVERTER_STATUS_STRUCTURAL, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index,
                      LogFormatStatusString(format_status));
            goto cleanup_failure;
        }
        if (have_previous_sequence && sample.sequence <= previous_sequence) {
            SetResult(result, LOG_CONVERTER_STATUS_STRUCTURAL, LOG_CONVERTER_FIELD_SEQUENCE, UINT32_MAX, record_index,
                      "sequence is not strictly increasing");
            goto cleanup_failure;
        }
        if (have_previous_sequence && sample.sequence > previous_sequence + 1U) {
            const uint64_t gap = sample.sequence - previous_sequence - 1U;

            if (UINT64_MAX - result->missing_sequences < gap) {
                SetResult(result, LOG_CONVERTER_STATUS_OVERFLOW, LOG_CONVERTER_FIELD_SEQUENCE, UINT32_MAX, record_index,
                          "missing sequence count overflows");
                goto cleanup_failure;
            }
            result->missing_sequences += gap;
        }
        if (!WriteSample(csv_file, &actual_descriptor, &sample)) {
            SetResult(result, LOG_CONVERTER_STATUS_OUTPUT_WRITE, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index, "could not write CSV record");
            goto cleanup_failure;
        }
        if (record_index == UINT64_MAX || result->exported_records == UINT64_MAX) {
            SetResult(result, LOG_CONVERTER_STATUS_OVERFLOW, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index, "record count overflows");
            goto cleanup_failure;
        }
        previous_sequence = sample.sequence;
        have_previous_sequence = true;
        ++record_index;
        ++result->exported_records;
    }
    if (fclose(csv_file) != 0) {
        csv_file = NULL;
        SetResult(result, LOG_CONVERTER_STATUS_OUTPUT_CLOSE, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index, "could not close temporary CSV");
        goto cleanup_failure;
    }
    csv_file = NULL;
    if (fclose(binary_file) != 0) {
        binary_file = NULL;
        SetResult(result, LOG_CONVERTER_STATUS_STRUCTURAL, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index, "could not close binary log");
        goto cleanup_failure;
    }
    binary_file = NULL;
    if (rename(temporary_path, csv_path) != 0) {
        SetResult(result, LOG_CONVERTER_STATUS_RENAME, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index, "could not publish CSV");
        goto cleanup_failure;
    }
    free(temporary_path);
    if (result->truncated) {
        SetResult(result, LOG_CONVERTER_STATUS_PARTIAL, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index,
                  "truncated tail: exported complete records only");
    } else {
        SetResult(result, LOG_CONVERTER_STATUS_OK, LOG_CONVERTER_FIELD_NONE, UINT32_MAX, record_index, "CSV export complete");
    }
    return result->status;

cleanup_failure:
    if (csv_file) (void)fclose(csv_file);
    if (binary_file) (void)fclose(binary_file);
    (void)unlink(temporary_path);
    free(temporary_path);
    return result->status;
}

const char *LogConverterStatusString(log_converter_status_t status) {
    switch (status) {
        case LOG_CONVERTER_STATUS_OK: return "ok";
        case LOG_CONVERTER_STATUS_PARTIAL: return "partial export";
        case LOG_CONVERTER_STATUS_INVALID_ARGUMENT: return "invalid argument";
        case LOG_CONVERTER_STATUS_RUNNING: return "execution is running";
        case LOG_CONVERTER_STATUS_INPUT_OPEN: return "input open failed";
        case LOG_CONVERTER_STATUS_HEADER: return "invalid log header";
        case LOG_CONVERTER_STATUS_SCHEMA_MISMATCH: return "schema mismatch";
        case LOG_CONVERTER_STATUS_TEMPORARY_OPEN: return "temporary CSV open failed";
        case LOG_CONVERTER_STATUS_OUTPUT_WRITE: return "CSV write failed";
        case LOG_CONVERTER_STATUS_OUTPUT_CLOSE: return "CSV close failed";
        case LOG_CONVERTER_STATUS_RENAME: return "CSV publish failed";
        case LOG_CONVERTER_STATUS_STRUCTURAL: return "structural log error";
        case LOG_CONVERTER_STATUS_OVERFLOW: return "overflow";
        default: return "unknown converter status";
    }
}

const char *LogConverterFieldString(log_converter_field_t field) {
    switch (field) {
        case LOG_CONVERTER_FIELD_NONE: return "none";
        case LOG_CONVERTER_FIELD_SHA256: return "sha256";
        case LOG_CONVERTER_FIELD_OUTPUT_COUNT: return "output_count";
        case LOG_CONVERTER_FIELD_XML_INDEX: return "xml_index";
        case LOG_CONVERTER_FIELD_VALUE_REFERENCE: return "value_reference";
        case LOG_CONVERTER_FIELD_TYPE: return "type";
        case LOG_CONVERTER_FIELD_NAME: return "name";
        case LOG_CONVERTER_FIELD_SEQUENCE: return "sequence";
        default: return "unknown field";
    }
}
