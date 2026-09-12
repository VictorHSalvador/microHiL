#ifndef DAQ_SCHEMA_H
#define DAQ_SCHEMA_H

/* Contrato público do módulo daq schema. */

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "input_state.h"

#define DAQ_SCHEMA_MAX_FIELDS MAX_OUTPUTS

typedef enum {
    DAQ_CHANNEL_AI = 0,
    DAQ_CHANNEL_DI,
    DAQ_CHANNEL_AO,
    DAQ_CHANNEL_DO,
    DAQ_CHANNEL_PWM
} daq_channel_function_t;

typedef enum {
    DAQ_WIRE_FLOAT32 = 0,
    DAQ_WIRE_INT32,
    DAQ_WIRE_BOOLEAN
} daq_wire_type_t;

typedef struct {
    uint8_t gpio;
    daq_channel_function_t function;
    daq_wire_type_t wire_type;
    NumericType fmu_type;
    uint16_t offset;
    uint16_t width;
    double scale;
    double offset_value;
    uint32_t value_reference;
    uint16_t fmu_index;
} daq_field_t;

typedef struct {
    daq_field_t fields[DAQ_SCHEMA_MAX_FIELDS];
    size_t field_count;
    size_t payload_size;
} daq_schema_t;

typedef enum {
    DAQ_SCHEMA_OK = 0,
    DAQ_SCHEMA_INVALID_ARGUMENT,
    DAQ_SCHEMA_TOO_MANY_FIELDS,
    DAQ_SCHEMA_INVALID_FUNCTION,
    DAQ_SCHEMA_INVALID_WIRE_TYPE,
    DAQ_SCHEMA_INVALID_FMU_TYPE,
    DAQ_SCHEMA_INVALID_OFFSET,
    DAQ_SCHEMA_DUPLICATE_GPIO,
    DAQ_SCHEMA_DUPLICATE_FMU_INDEX,
    DAQ_SCHEMA_PAYLOAD_TOO_LARGE,
    DAQ_SCHEMA_INVALID_SCALE
} daq_schema_status_t;

daq_schema_status_t DaqSchemaBuild(daq_schema_t *schema, const daq_field_t *fields, size_t field_count);
daq_schema_status_t DaqSchemaBuildFixedPayload(daq_schema_t *schema, const daq_field_t *fields, size_t field_count, size_t payload_size);
daq_schema_status_t DaqSchemaDecodeInput(const daq_field_t *field, const uint8_t *payload, size_t payload_size,
                                         input_value_t *value, bool *valid);
daq_schema_status_t DaqSchemaDecodeValue(const daq_field_t *field, const uint8_t *payload, size_t payload_size, double *value);
const char *DaqSchemaStatusString(daq_schema_status_t status);

#endif
