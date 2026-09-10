#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "daq_schema.h"

static void Require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    const daq_field_t fields[] = {
        {.gpio = 34U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL, .offset = 0U, .width = 4U, .scale = 2.0, .offset_value = -1.0, .fmu_index = 0U},
        {.gpio = 35U, .function = DAQ_CHANNEL_DI, .wire_type = DAQ_WIRE_BOOLEAN, .fmu_type = NUMERIC_BOOLEAN, .offset = 4U, .width = 1U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 1U}
    };
    const uint8_t payload[] = {0U, 0U, 0U, 0x3fU, 1U};
    daq_schema_t schema;
    double value = 0.0;
    Require(DaqSchemaBuild(&schema, fields, 2U) == DAQ_SCHEMA_OK && schema.payload_size == sizeof(payload), "valid schema was rejected");
    Require(DaqSchemaDecodeValue(&schema.fields[0], payload, sizeof(payload), &value) == DAQ_SCHEMA_OK && fabs(value - 0.0) < 1e-12, "float32 mapping differs");
    Require(DaqSchemaDecodeValue(&schema.fields[1], payload, sizeof(payload), &value) == DAQ_SCHEMA_OK && value == 1.0, "boolean mapping differs");
    daq_field_t duplicate = fields[1];
    duplicate.gpio = 34U;
    Require(DaqSchemaBuild(&schema, &duplicate, 1U) == DAQ_SCHEMA_INVALID_OFFSET, "noncanonical standalone field was accepted");
    return EXIT_SUCCESS;
}
