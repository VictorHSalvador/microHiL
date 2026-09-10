#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "daq_actuation.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    const daq_field_t fields[] = {
        {.gpio = 16U, .function = DAQ_CHANNEL_DO, .wire_type = DAQ_WIRE_BOOLEAN, .fmu_type = NUMERIC_BOOLEAN,
         .offset = 0U, .width = 1U, .scale = 1.0, .offset_value = 0.0, .value_reference = 6U, .fmu_index = 3U},
        {.gpio = 25U, .function = DAQ_CHANNEL_AO, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL,
         .offset = 13U, .width = 4U, .scale = 2.0, .offset_value = -1.0, .value_reference = 8U, .fmu_index = 2U}
    };
    const OutputVariable outputs[] = {
        {.name = "real_output", .xml_index = 5U, .value_reference = 8U, .fmu_index = 2U, .type = NUMERIC_REAL},
        {.name = "boolean_output", .xml_index = 7U, .value_reference = 6U, .fmu_index = 3U, .type = NUMERIC_BOOLEAN}
    };
    const log_value_t values[] = {{.real_value = 3.0}, {.boolean_value = 1U}};
    const log_value_t invalid_values[] = {{.real_value = NAN}, {.boolean_value = 2U}};
    daq_schema_t schema;
    daq_actuation_t actuation;
    uint8_t payload[21] = {0};

    Require(DaqSchemaBuildFixedPayload(&schema, fields, 2U, sizeof(payload)) == DAQ_SCHEMA_OK, "could not build actuation schema");
    Require(DaqActuationInit(&actuation, &schema) == DAQ_ACTUATION_OK, "could not initialize actuation encoder");
    Require(DaqActuationPack(&actuation, outputs, invalid_values, 2U, payload, sizeof(payload)) == DAQ_ACTUATION_OUTPUT_INVALID,
            "first invalid FMU output was accepted without a retained value");
    Require(DaqActuationPack(&actuation, outputs, values, 2U, payload, sizeof(payload)) == DAQ_ACTUATION_OK, "valid FMU outputs were rejected");
    Require(payload[0] == 1U && payload[13] == 0U && payload[14] == 0U && payload[15] == 0U && payload[16] == 0x40U,
            "FMU output transform was not encoded as little-endian float32");
    Require(DaqActuationPack(&actuation, outputs, invalid_values, 2U, payload, sizeof(payload)) == DAQ_ACTUATION_OK && payload[0] == 1U &&
            payload[16] == 0x40U, "invalid output did not retain the last valid actuation value");
    DaqActuationZero(&actuation, payload, sizeof(payload));
    for (size_t index = 0U; index < sizeof(payload); ++index) Require(payload[index] == 0U, "zero output payload contains a nonzero byte");
    return EXIT_SUCCESS;
}
