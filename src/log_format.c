#include "log_format.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#define LOG_FORMAT_VERSION UINT16_C(1)
#define LOG_FORMAT_FIXED_HEADER_SIZE UINT32_C(66)

static const uint8_t LOG_FORMAT_MAGIC[8] = {'M', 'H', 'I', 'L', 'L', 'O', 'G', '1'};

static log_format_status_t WriteBytes(FILE *file, const uint8_t *bytes, size_t byte_count) {
    if (fwrite(bytes, 1U, byte_count, file) != byte_count) return LOG_FORMAT_STATUS_IO_ERROR;
    return LOG_FORMAT_STATUS_OK;
}

static log_format_status_t ReadBytes(FILE *file, uint8_t *bytes, size_t byte_count, bool allow_empty_file) {
    const size_t read_count = fread(bytes, 1U, byte_count, file);

    if (read_count == byte_count) return LOG_FORMAT_STATUS_OK;
    if (ferror(file)) return LOG_FORMAT_STATUS_IO_ERROR;
    if (allow_empty_file && read_count == 0U && feof(file)) return LOG_FORMAT_STATUS_END_OF_FILE;
    return LOG_FORMAT_STATUS_TRUNCATED;
}

static log_format_status_t WriteU16(FILE *file, uint16_t value) {
    const uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8U)};
    return WriteBytes(file, bytes, sizeof(bytes));
}

static log_format_status_t WriteU32(FILE *file, uint32_t value) {
    const uint8_t bytes[4] = {
        (uint8_t)value, (uint8_t)(value >> 8U), (uint8_t)(value >> 16U), (uint8_t)(value >> 24U)
    };
    return WriteBytes(file, bytes, sizeof(bytes));
}

static log_format_status_t WriteU64(FILE *file, uint64_t value) {
    uint8_t bytes[8];

    for (size_t index = 0; index < sizeof(bytes); ++index) bytes[index] = (uint8_t)(value >> (index * 8U));
    return WriteBytes(file, bytes, sizeof(bytes));
}

static log_format_status_t ReadU16(FILE *file, uint16_t *value, bool allow_empty_file) {
    uint8_t bytes[2];
    const log_format_status_t status = ReadBytes(file, bytes, sizeof(bytes), allow_empty_file);

    if (status != LOG_FORMAT_STATUS_OK) return status;
    *value = (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
    return LOG_FORMAT_STATUS_OK;
}

static log_format_status_t ReadU32(FILE *file, uint32_t *value) {
    uint8_t bytes[4];
    const log_format_status_t status = ReadBytes(file, bytes, sizeof(bytes), false);

    if (status != LOG_FORMAT_STATUS_OK) return status;
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) | ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
    return LOG_FORMAT_STATUS_OK;
}

static log_format_status_t ReadU64Internal(FILE *file, uint64_t *value, bool allow_empty_file) {
    uint8_t bytes[8];
    const log_format_status_t status = ReadBytes(file, bytes, sizeof(bytes), allow_empty_file);

    if (status != LOG_FORMAT_STATUS_OK) return status;
    *value = UINT64_C(0);
    for (size_t index = 0; index < sizeof(bytes); ++index) *value |= (uint64_t)bytes[index] << (index * 8U);
    return LOG_FORMAT_STATUS_OK;
}

static log_format_status_t ReadU64(FILE *file, uint64_t *value) {
    return ReadU64Internal(file, value, false);
}

static log_format_status_t WriteDouble(FILE *file, double value) {
    uint64_t bits;

    memcpy(&bits, &value, sizeof(bits));
    return WriteU64(file, bits);
}

static log_format_status_t ReadDouble(FILE *file, double *value) {
    uint64_t bits;
    const log_format_status_t status = ReadU64(file, &bits);

    if (status != LOG_FORMAT_STATUS_OK) return status;
    memcpy(value, &bits, sizeof(*value));
    return LOG_FORMAT_STATUS_OK;
}

static bool IsValidType(log_value_type_t type) {
    return type == LOG_VALUE_REAL || type == LOG_VALUE_INTEGER || type == LOG_VALUE_BOOLEAN || type == LOG_VALUE_ENUMERATION;
}

static bool IsValidUtf8(const uint8_t *text, size_t text_size) {
    size_t index = 0U;

    while (index < text_size) {
        const uint8_t first = text[index];
        size_t continuation_count;
        uint32_t code_point;

        if (first <= UINT8_C(0x7f)) {
            if (first == 0U) return false;
            ++index;
            continue;
        }
        if (first >= UINT8_C(0xc2) && first <= UINT8_C(0xdf)) {
            continuation_count = 1U;
            code_point = (uint32_t)(first & UINT8_C(0x1f));
        } else if (first >= UINT8_C(0xe0) && first <= UINT8_C(0xef)) {
            continuation_count = 2U;
            code_point = (uint32_t)(first & UINT8_C(0x0f));
        } else if (first >= UINT8_C(0xf0) && first <= UINT8_C(0xf4)) {
            continuation_count = 3U;
            code_point = (uint32_t)(first & UINT8_C(0x07));
        } else {
            return false;
        }
        if (continuation_count > text_size - index - 1U) return false;
        for (size_t offset = 1U; offset <= continuation_count; ++offset) {
            const uint8_t next = text[index + offset];
            if ((next & UINT8_C(0xc0)) != UINT8_C(0x80)) return false;
            code_point = (code_point << 6U) | (uint32_t)(next & UINT8_C(0x3f));
        }
        if ((continuation_count == 2U && code_point < UINT32_C(0x800)) ||
            (continuation_count == 3U && code_point < UINT32_C(0x10000)) ||
            (code_point >= UINT32_C(0xd800) && code_point <= UINT32_C(0xdfff)) || code_point > UINT32_C(0x10ffff)) {
            return false;
        }
        index += continuation_count + 1U;
    }
    return true;
}

static log_format_status_t OutputNameLength(const log_output_t *output, uint16_t *name_size) {
    size_t length = 0U;

    while (length < sizeof(output->name) && output->name[length] != '\0') ++length;

    if (length == 0U || length == sizeof(output->name)) return LOG_FORMAT_STATUS_LIMIT;
    if (!IsValidUtf8((const uint8_t *)output->name, length)) return LOG_FORMAT_STATUS_UTF8;
    *name_size = (uint16_t)length;
    return LOG_FORMAT_STATUS_OK;
}

static log_format_status_t ValidateDescriptor(const log_descriptor_t *descriptor, uint32_t *header_size) {
    uint64_t size = LOG_FORMAT_FIXED_HEADER_SIZE;

    if (!descriptor || !header_size) return LOG_FORMAT_STATUS_INVALID_ARGUMENT;
    if (descriptor->output_count == 0U || descriptor->output_count > LOG_FORMAT_MAX_OUTPUTS || !isfinite(descriptor->t_start_s) ||
        !isfinite(descriptor->step_size_s) || descriptor->step_size_s <= 0.0) {
        return LOG_FORMAT_STATUS_LIMIT;
    }
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        uint16_t name_size;
        const log_output_t *output = &descriptor->outputs[index];
        const log_format_status_t status = OutputNameLength(output, &name_size);

        if (status != LOG_FORMAT_STATUS_OK) return status;
        if (!IsValidType(output->type)) return LOG_FORMAT_STATUS_TYPE;
        if (index > 0U && output->xml_index <= descriptor->outputs[index - 1U].xml_index) return LOG_FORMAT_STATUS_ORDER;
        size += UINT64_C(11) + (uint64_t)name_size;
        if (size > UINT32_MAX) return LOG_FORMAT_STATUS_OVERFLOW;
    }
    *header_size = (uint32_t)size;
    return LOG_FORMAT_STATUS_OK;
}

size_t LogFormatQualityBytes(uint32_t output_count) {
    if (output_count > LOG_FORMAT_MAX_OUTPUTS) return 0U;
    return ((size_t)output_count + 7U) / 8U;
}

log_format_status_t LogFormatHeaderSize(const log_descriptor_t *descriptor, uint32_t *header_size) {
    return ValidateDescriptor(descriptor, header_size);
}

log_format_status_t LogFormatWriteHeader(FILE *file, const log_descriptor_t *descriptor) {
    uint32_t header_size;
    log_format_status_t status;

    if (!file) return LOG_FORMAT_STATUS_INVALID_ARGUMENT;
    status = ValidateDescriptor(descriptor, &header_size);
    if (status != LOG_FORMAT_STATUS_OK) return status;
    if ((status = WriteBytes(file, LOG_FORMAT_MAGIC, sizeof(LOG_FORMAT_MAGIC))) != LOG_FORMAT_STATUS_OK ||
        (status = WriteU16(file, LOG_FORMAT_VERSION)) != LOG_FORMAT_STATUS_OK ||
        (status = WriteU32(file, header_size)) != LOG_FORMAT_STATUS_OK ||
        (status = WriteBytes(file, descriptor->fmu_sha256, sizeof(descriptor->fmu_sha256))) != LOG_FORMAT_STATUS_OK ||
        (status = WriteDouble(file, descriptor->t_start_s)) != LOG_FORMAT_STATUS_OK ||
        (status = WriteDouble(file, descriptor->step_size_s)) != LOG_FORMAT_STATUS_OK ||
        (status = WriteU32(file, descriptor->output_count)) != LOG_FORMAT_STATUS_OK) {
        return status;
    }
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        const log_output_t *output = &descriptor->outputs[index];
        uint16_t name_size;
        const uint8_t type_code = (uint8_t)output->type;

        status = OutputNameLength(output, &name_size);
        if (status != LOG_FORMAT_STATUS_OK || (status = WriteU32(file, output->xml_index)) != LOG_FORMAT_STATUS_OK ||
            (status = WriteU32(file, output->value_reference)) != LOG_FORMAT_STATUS_OK ||
            (status = WriteBytes(file, &type_code, 1U)) != LOG_FORMAT_STATUS_OK ||
            (status = WriteU16(file, name_size)) != LOG_FORMAT_STATUS_OK ||
            (status = WriteBytes(file, (const uint8_t *)output->name, name_size)) != LOG_FORMAT_STATUS_OK) {
            return status;
        }
    }
    return LOG_FORMAT_STATUS_OK;
}

log_format_status_t LogFormatReadHeader(FILE *file, log_descriptor_t *descriptor) {
    uint8_t magic[sizeof(LOG_FORMAT_MAGIC)];
    uint16_t version;
    uint32_t header_size;
    uint64_t consumed = LOG_FORMAT_FIXED_HEADER_SIZE;
    log_format_status_t status;

    if (!file || !descriptor) return LOG_FORMAT_STATUS_INVALID_ARGUMENT;
    memset(descriptor, 0, sizeof(*descriptor));
    if ((status = ReadBytes(file, magic, sizeof(magic), false)) != LOG_FORMAT_STATUS_OK) return status;
    if (memcmp(magic, LOG_FORMAT_MAGIC, sizeof(magic)) != 0) return LOG_FORMAT_STATUS_MAGIC;
    if ((status = ReadU16(file, &version, false)) != LOG_FORMAT_STATUS_OK) return status;
    if (version != LOG_FORMAT_VERSION) return LOG_FORMAT_STATUS_VERSION;
    if ((status = ReadU32(file, &header_size)) != LOG_FORMAT_STATUS_OK ||
        (status = ReadBytes(file, descriptor->fmu_sha256, sizeof(descriptor->fmu_sha256), false)) != LOG_FORMAT_STATUS_OK ||
        (status = ReadDouble(file, &descriptor->t_start_s)) != LOG_FORMAT_STATUS_OK ||
        (status = ReadDouble(file, &descriptor->step_size_s)) != LOG_FORMAT_STATUS_OK ||
        (status = ReadU32(file, &descriptor->output_count)) != LOG_FORMAT_STATUS_OK) {
        return status;
    }
    if (header_size < LOG_FORMAT_FIXED_HEADER_SIZE) return LOG_FORMAT_STATUS_HEADER_SIZE;
    if (descriptor->output_count == 0U || descriptor->output_count > LOG_FORMAT_MAX_OUTPUTS || !isfinite(descriptor->t_start_s) ||
        !isfinite(descriptor->step_size_s) || descriptor->step_size_s <= 0.0) {
        return LOG_FORMAT_STATUS_LIMIT;
    }
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        log_output_t *output = &descriptor->outputs[index];
        uint8_t type_code;
        uint16_t name_size;

        if ((status = ReadU32(file, &output->xml_index)) != LOG_FORMAT_STATUS_OK ||
            (status = ReadU32(file, &output->value_reference)) != LOG_FORMAT_STATUS_OK ||
            (status = ReadBytes(file, &type_code, 1U, false)) != LOG_FORMAT_STATUS_OK ||
            (status = ReadU16(file, &name_size, false)) != LOG_FORMAT_STATUS_OK) {
            return status;
        }
        consumed += UINT64_C(11) + (uint64_t)name_size;
        if (consumed > header_size) return LOG_FORMAT_STATUS_HEADER_SIZE;
        if (name_size == 0U || name_size > LOG_FORMAT_MAX_NAME_BYTES) return LOG_FORMAT_STATUS_LIMIT;
        if (type_code > (uint8_t)LOG_VALUE_ENUMERATION) return LOG_FORMAT_STATUS_TYPE;
        output->type = (log_value_type_t)type_code;
        if (index > 0U && output->xml_index <= descriptor->outputs[index - 1U].xml_index) return LOG_FORMAT_STATUS_ORDER;
        if ((status = ReadBytes(file, (uint8_t *)output->name, name_size, false)) != LOG_FORMAT_STATUS_OK) return status;
        output->name[name_size] = '\0';
        if (!IsValidUtf8((const uint8_t *)output->name, name_size)) return LOG_FORMAT_STATUS_UTF8;
    }
    if (consumed != header_size) return LOG_FORMAT_STATUS_HEADER_SIZE;
    return LOG_FORMAT_STATUS_OK;
}

static uint8_t QualityMask(uint32_t output_count) {
    const uint32_t used_bits = output_count % 8U;

    if (used_bits == 0U) return UINT8_MAX;
    return (uint8_t)((UINT32_C(1) << used_bits) - UINT32_C(1));
}

static bool IsInvalid(const uint8_t *quality_bitmap, uint32_t index) {
    return (quality_bitmap[index / 8U] & (uint8_t)(UINT32_C(1) << (index % 8U))) != 0U;
}

static void MarkInvalid(uint8_t *quality_bitmap, uint32_t index) {
    quality_bitmap[index / 8U] |= (uint8_t)(UINT32_C(1) << (index % 8U));
}

static log_format_status_t WriteValue(FILE *file, log_value_type_t type, const log_value_t *value, bool invalid) {
    if (type == LOG_VALUE_REAL) return WriteDouble(file, invalid ? NAN : value->real_value);
    if (type == LOG_VALUE_BOOLEAN) {
        const uint8_t boolean_value = invalid ? 0U : value->boolean_value;
        return WriteBytes(file, &boolean_value, 1U);
    }
    return WriteU32(file, invalid ? 0U : (uint32_t)(int32_t)value->discrete_value);
}

log_format_status_t LogFormatWriteRecord(FILE *file, const log_descriptor_t *descriptor, const log_sample_t *sample) {
    uint32_t ignored_header_size;
    uint8_t quality_bitmap[LOG_FORMAT_QUALITY_BYTES] = {0};
    const size_t quality_bytes = LogFormatQualityBytes(descriptor ? descriptor->output_count : UINT32_MAX);
    log_format_status_t status;

    if (!file || !sample || quality_bytes == 0U) return LOG_FORMAT_STATUS_INVALID_ARGUMENT;
    status = ValidateDescriptor(descriptor, &ignored_header_size);
    if (status != LOG_FORMAT_STATUS_OK || !isfinite(sample->sim_time_s)) return status != LOG_FORMAT_STATUS_OK ? status : LOG_FORMAT_STATUS_INCONSISTENT;
    memcpy(quality_bitmap, sample->quality_bitmap, quality_bytes);
    quality_bitmap[quality_bytes - 1U] &= QualityMask(descriptor->output_count);
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        const log_value_type_t type = descriptor->outputs[index].type;
        const log_value_t *value = &sample->values[index];

        if (type == LOG_VALUE_REAL && !isfinite(value->real_value)) MarkInvalid(quality_bitmap, index);
        if ((type == LOG_VALUE_INTEGER || type == LOG_VALUE_ENUMERATION) &&
            (value->discrete_value < INT32_MIN || value->discrete_value > INT32_MAX)) {
            MarkInvalid(quality_bitmap, index);
        }
        if (type == LOG_VALUE_BOOLEAN && value->boolean_value > 1U) MarkInvalid(quality_bitmap, index);
    }
    if ((status = WriteU64(file, sample->sequence)) != LOG_FORMAT_STATUS_OK ||
        (status = WriteDouble(file, sample->sim_time_s)) != LOG_FORMAT_STATUS_OK ||
        (status = WriteBytes(file, quality_bitmap, quality_bytes)) != LOG_FORMAT_STATUS_OK) {
        return status;
    }
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        status = WriteValue(file, descriptor->outputs[index].type, &sample->values[index], IsInvalid(quality_bitmap, index));
        if (status != LOG_FORMAT_STATUS_OK) return status;
    }
    return LOG_FORMAT_STATUS_OK;
}

static int32_t DecodeI32(uint32_t bits) {
    if (bits <= INT32_MAX) return (int32_t)bits;
    return (int32_t)((int64_t)bits - (int64_t)UINT32_MAX - INT64_C(1));
}

static log_format_status_t ReadValue(FILE *file, log_value_type_t type, log_value_t *value) {
    uint32_t bits;
    log_format_status_t status;

    if (type == LOG_VALUE_REAL) return ReadDouble(file, &value->real_value);
    if (type == LOG_VALUE_BOOLEAN) return ReadBytes(file, &value->boolean_value, 1U, false);
    status = ReadU32(file, &bits);
    if (status != LOG_FORMAT_STATUS_OK) return status;
    value->discrete_value = DecodeI32(bits);
    return LOG_FORMAT_STATUS_OK;
}

log_format_status_t LogFormatReadRecord(FILE *file, const log_descriptor_t *descriptor, log_sample_t *sample) {
    uint32_t ignored_header_size;
    const size_t quality_bytes = LogFormatQualityBytes(descriptor ? descriptor->output_count : UINT32_MAX);
    log_format_status_t status;

    if (!file || !sample || quality_bytes == 0U) return LOG_FORMAT_STATUS_INVALID_ARGUMENT;
    status = ValidateDescriptor(descriptor, &ignored_header_size);
    if (status != LOG_FORMAT_STATUS_OK) return status;
    memset(sample, 0, sizeof(*sample));
    status = ReadU64Internal(file, &sample->sequence, true);
    if (status == LOG_FORMAT_STATUS_END_OF_FILE) return status;
    if (status != LOG_FORMAT_STATUS_OK || (status = ReadDouble(file, &sample->sim_time_s)) != LOG_FORMAT_STATUS_OK ||
        (status = ReadBytes(file, sample->quality_bitmap, quality_bytes, false)) != LOG_FORMAT_STATUS_OK) {
        return status;
    }
    if (!isfinite(sample->sim_time_s) || (sample->quality_bitmap[quality_bytes - 1U] & (uint8_t)~QualityMask(descriptor->output_count)) != 0U) {
        return LOG_FORMAT_STATUS_INCONSISTENT;
    }
    for (uint32_t index = 0U; index < descriptor->output_count; ++index) {
        const bool invalid = IsInvalid(sample->quality_bitmap, index);
        const log_value_type_t type = descriptor->outputs[index].type;

        if ((status = ReadValue(file, type, &sample->values[index])) != LOG_FORMAT_STATUS_OK) return status;
        if ((type == LOG_VALUE_REAL && ((invalid && !isnan(sample->values[index].real_value)) || (!invalid && !isfinite(sample->values[index].real_value)))) ||
            ((type == LOG_VALUE_INTEGER || type == LOG_VALUE_ENUMERATION) && invalid && sample->values[index].discrete_value != 0) ||
            (type == LOG_VALUE_BOOLEAN && (sample->values[index].boolean_value > 1U || (invalid && sample->values[index].boolean_value != 0U)))) {
            return LOG_FORMAT_STATUS_INCONSISTENT;
        }
    }
    return LOG_FORMAT_STATUS_OK;
}

const char *LogFormatStatusString(log_format_status_t status) {
    switch (status) {
        case LOG_FORMAT_STATUS_OK: return "ok";
        case LOG_FORMAT_STATUS_END_OF_FILE: return "end of file";
        case LOG_FORMAT_STATUS_INVALID_ARGUMENT: return "invalid argument";
        case LOG_FORMAT_STATUS_IO_ERROR: return "I/O error";
        case LOG_FORMAT_STATUS_TRUNCATED: return "truncated input";
        case LOG_FORMAT_STATUS_MAGIC: return "invalid magic";
        case LOG_FORMAT_STATUS_VERSION: return "unsupported version";
        case LOG_FORMAT_STATUS_HEADER_SIZE: return "invalid header size";
        case LOG_FORMAT_STATUS_LIMIT: return "format limit exceeded";
        case LOG_FORMAT_STATUS_OVERFLOW: return "size overflow";
        case LOG_FORMAT_STATUS_TYPE: return "invalid value type";
        case LOG_FORMAT_STATUS_ORDER: return "invalid XML index order";
        case LOG_FORMAT_STATUS_UTF8: return "invalid UTF-8 name";
        case LOG_FORMAT_STATUS_INCONSISTENT: return "inconsistent record";
        default: return "unknown log format status";
    }
}
