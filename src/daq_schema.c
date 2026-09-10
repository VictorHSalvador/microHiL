#include "daq_schema.h"

#include <math.h>
#include <string.h>

#include "daq_protocol.h"

static size_t WireWidth(daq_wire_type_t wire_type) {
    switch (wire_type) {
        case DAQ_WIRE_FLOAT32:
        case DAQ_WIRE_INT32: return 4U;
        case DAQ_WIRE_BOOLEAN: return 1U;
        default: return 0U;
    }
}

static bool IsInputFunction(daq_channel_function_t function) {
    return function == DAQ_CHANNEL_AI || function == DAQ_CHANNEL_DI;
}

static bool IsTypeCompatible(const daq_field_t *field) {
    if (field->wire_type == DAQ_WIRE_FLOAT32) return field->fmu_type == NUMERIC_REAL;
    if (field->wire_type == DAQ_WIRE_INT32) return field->fmu_type == NUMERIC_INTEGER || field->fmu_type == NUMERIC_ENUMERATION;
    return field->wire_type == DAQ_WIRE_BOOLEAN && field->fmu_type == NUMERIC_BOOLEAN;
}

static uint32_t ReadLe32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) | ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

daq_schema_status_t DaqSchemaBuild(daq_schema_t *schema, const daq_field_t *fields, size_t field_count) {
    if (!schema || (field_count > 0U && !fields)) return DAQ_SCHEMA_INVALID_ARGUMENT;
    if (field_count > DAQ_SCHEMA_MAX_FIELDS) return DAQ_SCHEMA_TOO_MANY_FIELDS;
    *schema = (daq_schema_t){0};

    size_t payload_size = 0U;
    for (size_t index = 0U; index < field_count; ++index) {
        const daq_field_t *field = &fields[index];
        const size_t width = WireWidth(field->wire_type);
        if (field->function > DAQ_CHANNEL_PWM) return DAQ_SCHEMA_INVALID_FUNCTION;
        if (width == 0U) return DAQ_SCHEMA_INVALID_WIRE_TYPE;
        if (!IsTypeCompatible(field)) return DAQ_SCHEMA_INVALID_FMU_TYPE;
        if (!isfinite(field->scale) || !isfinite(field->offset_value) || field->scale == 0.0) return DAQ_SCHEMA_INVALID_SCALE;
        if (field->width != width || field->offset != payload_size) return DAQ_SCHEMA_INVALID_OFFSET;
        for (size_t prior = 0U; prior < index; ++prior) {
            if (fields[prior].gpio == field->gpio) return DAQ_SCHEMA_DUPLICATE_GPIO;
        }
        payload_size += width;
        if (payload_size > DAQ_PROTOCOL_MAX_DATA_PAYLOAD) return DAQ_SCHEMA_PAYLOAD_TOO_LARGE;
        schema->fields[index] = *field;
    }
    schema->field_count = field_count;
    schema->payload_size = payload_size;
    return DAQ_SCHEMA_OK;
}

daq_schema_status_t DaqSchemaDecodeValue(const daq_field_t *field, const uint8_t *payload, size_t payload_size, double *value) {
    if (!field || !payload || !value || !IsInputFunction(field->function) || field->offset + field->width > payload_size) return DAQ_SCHEMA_INVALID_ARGUMENT;
    if (field->wire_type == DAQ_WIRE_BOOLEAN) {
        if (payload[field->offset] > UINT8_C(1)) return DAQ_SCHEMA_INVALID_ARGUMENT;
        *value = (double)payload[field->offset];
        return DAQ_SCHEMA_OK;
    }
    if (field->wire_type == DAQ_WIRE_INT32) {
        const int32_t raw = (int32_t)ReadLe32(payload + field->offset);
        *value = (double)raw * field->scale + field->offset_value;
        return DAQ_SCHEMA_OK;
    }
    if (field->wire_type == DAQ_WIRE_FLOAT32) {
        float raw = 0.0F;
        const uint32_t raw_bits = ReadLe32(payload + field->offset);
        memcpy(&raw, &raw_bits, sizeof(raw));
        *value = (double)raw * field->scale + field->offset_value;
        return isfinite(*value) ? DAQ_SCHEMA_OK : DAQ_SCHEMA_INVALID_ARGUMENT;
    }
    return DAQ_SCHEMA_INVALID_WIRE_TYPE;
}

const char *DaqSchemaStatusString(daq_schema_status_t status) {
    switch (status) {
        case DAQ_SCHEMA_OK: return "ok";
        case DAQ_SCHEMA_INVALID_ARGUMENT: return "invalid argument";
        case DAQ_SCHEMA_TOO_MANY_FIELDS: return "too many fields";
        case DAQ_SCHEMA_INVALID_FUNCTION: return "invalid channel function";
        case DAQ_SCHEMA_INVALID_WIRE_TYPE: return "invalid wire type";
        case DAQ_SCHEMA_INVALID_FMU_TYPE: return "wire and FMU types are incompatible";
        case DAQ_SCHEMA_INVALID_OFFSET: return "field offsets are not canonical";
        case DAQ_SCHEMA_DUPLICATE_GPIO: return "GPIO used by multiple fields";
        case DAQ_SCHEMA_PAYLOAD_TOO_LARGE: return "payload exceeds 256 bytes";
        case DAQ_SCHEMA_INVALID_SCALE: return "invalid scale";
        default: return "unknown schema status";
    }
}
