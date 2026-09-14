/* Implementação do módulo daq actuation. */
#include "daq_actuation.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define DAQ_AO_MIN_VOLTS 0.0
#define DAQ_AO_MAX_VOLTS 3.3
#define DAQ_PWM_MIN_DUTY 0.0
#define DAQ_PWM_MAX_DUTY 1.0

static bool IsActuationFunction(daq_channel_function_t function) {
    return function == DAQ_CHANNEL_AO || function == DAQ_CHANNEL_DO || function == DAQ_CHANNEL_PWM;
}

static bool IsPhysicalRangeValid(daq_channel_function_t function, double raw_value) {
    if (function == DAQ_CHANNEL_AO) return raw_value >= DAQ_AO_MIN_VOLTS && raw_value <= DAQ_AO_MAX_VOLTS;
    if (function == DAQ_CHANNEL_PWM) return raw_value >= DAQ_PWM_MIN_DUTY && raw_value <= DAQ_PWM_MAX_DUTY;
    return true;
}

static void WriteLe32(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)(value & UINT32_C(0x000000ff));
    bytes[1] = (uint8_t)((value >> 8U) & UINT32_C(0x000000ff));
    bytes[2] = (uint8_t)((value >> 16U) & UINT32_C(0x000000ff));
    bytes[3] = (uint8_t)((value >> 24U) & UINT32_C(0x000000ff));
}

static daq_actuation_status_t EncodeField(const daq_field_t *field, const log_value_t *value, uint8_t *payload) {
    if (field->wire_type == DAQ_WIRE_BOOLEAN) {
        if (value->boolean_value > 1U) return DAQ_ACTUATION_OUTPUT_INVALID;
        payload[field->offset] = value->boolean_value;
        return DAQ_ACTUATION_OK;
    }
    if (field->wire_type != DAQ_WIRE_FLOAT32 || !isfinite(value->real_value)) return DAQ_ACTUATION_OUTPUT_INVALID;
    const double raw_value = (value->real_value - field->offset_value) / field->scale;
    if (!isfinite(raw_value) || raw_value < -(double)FLT_MAX || raw_value > (double)FLT_MAX || !IsPhysicalRangeValid(field->function, raw_value)) {
        return DAQ_ACTUATION_OUTPUT_RANGE;
    }
    const float raw_float = (float)raw_value;
    uint32_t raw_bits = 0U;
    memcpy(&raw_bits, &raw_float, sizeof(raw_bits));
    WriteLe32(payload + field->offset, raw_bits);
    return DAQ_ACTUATION_OK;
}

static const log_value_t *FindOutputValue(const daq_field_t *field, const OutputVariable *outputs, const log_value_t *values, size_t value_count) {
    for (size_t index = 0U; index < value_count; ++index) {
        if (outputs[index].fmu_index == field->fmu_index && outputs[index].value_reference == field->value_reference &&
            outputs[index].type == field->fmu_type) return &values[index];
    }
    return NULL;
}

daq_actuation_status_t DaqActuationInit(daq_actuation_t *actuation, const daq_schema_t *schema) {
    if (!actuation || !schema || schema->payload_size > DAQ_PROTOCOL_MAX_DATA_PAYLOAD) return DAQ_ACTUATION_INVALID_ARGUMENT;
    for (size_t index = 0U; index < schema->field_count; ++index) {
        if (!IsActuationFunction(schema->fields[index].function)) return DAQ_ACTUATION_INCOMPATIBLE_SCHEMA;
    }
    *actuation = (daq_actuation_t){.schema = schema, .initialized = true};
    return DAQ_ACTUATION_OK;
}

daq_actuation_status_t DaqActuationPack(daq_actuation_t *actuation, const OutputVariable *outputs, const log_value_t *values, size_t value_count,
                                        uint8_t *payload, size_t payload_capacity) {
    if (!actuation || !actuation->initialized || !outputs || !values || !payload || payload_capacity < actuation->schema->payload_size) {
        return DAQ_ACTUATION_INVALID_ARGUMENT;
    }
    uint8_t next_payload[DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    bool next_has_last_value[DAQ_SCHEMA_MAX_FIELDS];
    memcpy(next_payload, actuation->last_payload, actuation->schema->payload_size);
    memcpy(next_has_last_value, actuation->field_has_last_value, sizeof(next_has_last_value));
    for (size_t index = 0U; index < actuation->schema->field_count; ++index) {
        const daq_field_t *field = &actuation->schema->fields[index];
        const log_value_t *value = FindOutputValue(field, outputs, values, value_count);
        if (!value) return DAQ_ACTUATION_INVALID_ARGUMENT;
        const daq_actuation_status_t status = EncodeField(field, value, next_payload);
        if (status == DAQ_ACTUATION_OK) {
            next_has_last_value[index] = true;
        } else if (!next_has_last_value[index]) {
            return status;
        } else {
            memcpy(next_payload + field->offset, actuation->last_payload + field->offset, field->width);
        }
    }
    memcpy(actuation->last_payload, next_payload, actuation->schema->payload_size);
    memcpy(actuation->field_has_last_value, next_has_last_value, sizeof(next_has_last_value));
    memcpy(payload, next_payload, actuation->schema->payload_size);
    return DAQ_ACTUATION_OK;
}

void DaqActuationZero(const daq_actuation_t *actuation, uint8_t *payload, size_t payload_capacity) {
    if (!actuation || !actuation->initialized || !payload || payload_capacity < actuation->schema->payload_size) return;
    memset(payload, 0, actuation->schema->payload_size);
}

const char *DaqActuationStatusString(daq_actuation_status_t status) {
    switch (status) {
        case DAQ_ACTUATION_OK: return "ok";
        case DAQ_ACTUATION_INVALID_ARGUMENT: return "invalid argument";
        case DAQ_ACTUATION_INCOMPATIBLE_SCHEMA: return "schema is incompatible with FMU outputs";
        case DAQ_ACTUATION_OUTPUT_INVALID: return "output value is invalid";
        case DAQ_ACTUATION_OUTPUT_RANGE: return "output value cannot be represented on the wire";
        default: return "unknown actuation status";
    }
}
