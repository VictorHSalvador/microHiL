#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "daq_acquisition.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    const input_channel_descriptor_t inputs[] = {
        {.input_name = "voltage", .value_reference = 1U, .type = NUMERIC_REAL, .initial_value = {.real_value = -1.0}, .initial_value_valid = true},
        {.input_name = "enabled", .value_reference = 2U, .type = NUMERIC_BOOLEAN, .initial_value = {.boolean_value = 0U}, .initial_value_valid = true}
    };
    const daq_field_t fields[] = {
        {.gpio = 34U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL,
         .offset = 0U, .width = 4U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 0U},
        {.gpio = 35U, .function = DAQ_CHANNEL_DI, .wire_type = DAQ_WIRE_BOOLEAN, .fmu_type = NUMERIC_BOOLEAN,
         .offset = 4U, .width = 1U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 1U}
    };
    const uint8_t valid_payload[] = {0U, 0U, 0x60U, 0x40U, 1U};
    const uint8_t invalid_payload[] = {0U, 0U, 0xc0U, 0x7fU, 2U};
    daq_schema_t schema;
    input_state_t input_state;
    input_step_t step;
    daq_acquisition_t acquisition;

    Require(DaqSchemaBuild(&schema, fields, 2U) == DAQ_SCHEMA_OK, "could not build acquisition schema");
    Require(InputStateInit(&input_state, inputs, 2U, true) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    Require(DaqAcquisitionInit(&acquisition, &schema, &input_state) == DAQ_ACQUISITION_OK, "could not initialize acquisition bridge");
    Require(DaqAcquisitionApply(&acquisition, valid_payload, sizeof(valid_payload)) == DAQ_ACQUISITION_OK, "valid acquisition payload was rejected");
    Require(InputStatePrepareStep(&input_state, &step) == INPUT_STATE_STATUS_OK && fabs(step.values[0].real_value - 3.5) < 1e-12 &&
            step.values[1].boolean_value == 1U, "acquisition values were not published to the FMU snapshot");
    Require(DaqAcquisitionApply(&acquisition, invalid_payload, sizeof(invalid_payload)) == DAQ_ACQUISITION_OK,
            "invalid per-channel values rejected the complete frame");
    Require(InputStatePrepareStep(&input_state, &step) == INPUT_STATE_STATUS_OK && fabs(step.values[0].real_value - 3.5) < 1e-12 &&
            step.values[1].boolean_value == 1U && acquisition.invalid_values == 2U,
            "invalid acquisition values did not retain the prior FMU snapshot");
    Require(DaqAcquisitionApply(&acquisition, valid_payload, 4U) == DAQ_ACQUISITION_PAYLOAD_SIZE,
            "truncated acquisition payload was accepted");
    InputStateDestroy(&input_state);
    return EXIT_SUCCESS;
}
