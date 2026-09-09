#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input_state.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static input_channel_descriptor_t MakeRealDescriptor(void) {
    return (input_channel_descriptor_t){.input_name = "u_real", .value_reference = 3U, .type = NUMERIC_REAL,
                                        .initial_value = {.real_value = 1.25}, .initial_value_valid = true};
}

static input_channel_descriptor_t MakeBooleanDescriptor(void) {
    return (input_channel_descriptor_t){.input_name = "u_boolean", .value_reference = 0U, .type = NUMERIC_BOOLEAN,
                                        .initial_value = {.boolean_value = 0U}, .initial_value_valid = true};
}

static void TestRetainsLastValidAndUsesInitialValue(void) {
    input_state_t state;
    input_step_t step;
    input_value_t value = {.real_value = NAN};
    const input_channel_descriptor_t descriptor = MakeRealDescriptor();

    Require(InputStateInit(&state, &descriptor, 1U, true) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    Require(InputStatePrepareStep(&state, &step) == INPUT_STATE_STATUS_OK && step.values[0].real_value == 1.25,
            "missing acquisition did not use the FMU initial value");
    Require(InputStatePublish(&state, 0U, value, true) == INPUT_STATE_STATUS_OK, "could not publish invalid input");
    Require(InputStatePrepareStep(&state, &step) == INPUT_STATE_STATUS_OK && step.values[0].real_value == 1.25,
            "invalid first acquisition did not retain the initial value");
    value.real_value = 3.0;
    Require(InputStatePublish(&state, 0U, value, true) == INPUT_STATE_STATUS_OK, "could not publish valid input");
    Require(InputStatePrepareStep(&state, &step) == INPUT_STATE_STATUS_OK && step.values[0].real_value == 3.0,
            "valid acquisition was not selected");
    value.real_value = INFINITY;
    Require(InputStatePublish(&state, 0U, value, true) == INPUT_STATE_STATUS_OK, "could not publish infinity");
    Require(InputStatePrepareStep(&state, &step) == INPUT_STATE_STATUS_OK && step.values[0].real_value == 3.0,
            "invalid acquisition overwrote the last valid value");
    InputStateDestroy(&state);
}

static void TestUsesOnlyLatestUpdateAndDoesNotCountAbsence(void) {
    input_state_t state;
    input_step_t step;
    input_value_t value = {.real_value = 2.0};
    const input_channel_descriptor_t descriptor = MakeRealDescriptor();

    Require(InputStateInit(&state, &descriptor, 1U, true) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    Require(InputStatePublish(&state, 0U, value, true) == INPUT_STATE_STATUS_OK, "could not publish first input");
    value.real_value = 4.0;
    Require(InputStatePublish(&state, 0U, value, true) == INPUT_STATE_STATUS_OK, "could not publish latest input");
    Require(InputStatePrepareStep(&state, &step) == INPUT_STATE_STATUS_OK && step.values[0].real_value == 4.0,
            "the last input before the step was not selected");
    Require(InputStatePrepareStep(&state, &step) == INPUT_STATE_STATUS_OK && step.values[0].real_value == 4.0,
            "absence of a packet changed the retained value");
    InputStateDestroy(&state);
}

static void TestProtectionCountsPerStepAndCanBeDisabled(void) {
    input_state_t enabled;
    input_state_t disabled;
    input_step_t step;
    input_value_t invalid = {.boolean_value = 2U};
    input_value_t valid = {.boolean_value = 1U};
    const input_channel_descriptor_t descriptor = MakeBooleanDescriptor();

    Require(InputStateInit(&enabled, &descriptor, 1U, true) == INPUT_STATE_STATUS_OK, "could not initialize enabled protection");
    for (size_t index = 0U; index < INPUT_STATE_INVALID_STEP_LIMIT - 1U; ++index) {
        Require(InputStatePublish(&enabled, 0U, invalid, true) == INPUT_STATE_STATUS_OK, "could not publish invalid boolean");
        Require(InputStatePrepareStep(&enabled, &step) == INPUT_STATE_STATUS_OK, "protection triggered before 100 invalid steps");
    }
    Require(InputStatePublish(&enabled, 0U, invalid, true) == INPUT_STATE_STATUS_OK, "could not publish 100th invalid boolean");
    Require(InputStatePrepareStep(&enabled, &step) == INPUT_STATE_STATUS_PROTECTION_TRIPPED && step.protection_tripped &&
            step.invalid_limit_channel_count == 1U && step.invalid_limit_channels[0] == 0U,
            "protection did not trigger on the 100th invalid step");
    InputStateDestroy(&enabled);

    Require(InputStateInit(&disabled, &descriptor, 1U, false) == INPUT_STATE_STATUS_OK, "could not initialize disabled protection");
    for (size_t index = 0U; index < INPUT_STATE_INVALID_STEP_LIMIT + 1U; ++index) {
        Require(InputStatePublish(&disabled, 0U, invalid, true) == INPUT_STATE_STATUS_OK, "could not publish invalid value with protection disabled");
        Require(InputStatePrepareStep(&disabled, &step) == INPUT_STATE_STATUS_OK, "disabled protection ended the run");
    }
    Require(InputStatePublish(&disabled, 0U, valid, true) == INPUT_STATE_STATUS_OK, "could not publish valid boolean");
    Require(InputStatePrepareStep(&disabled, &step) == INPUT_STATE_STATUS_OK && step.values[0].boolean_value == 1U,
            "valid constant was not accepted after invalid values");
    InputStateDestroy(&disabled);
}

int main(int argc, char **argv) {
    if (argc != 2) return EXIT_FAILURE;
    if (strcmp(argv[1], "retention") == 0) TestRetainsLastValidAndUsesInitialValue();
    else if (strcmp(argv[1], "snapshot") == 0) TestUsesOnlyLatestUpdateAndDoesNotCountAbsence();
    else if (strcmp(argv[1], "protection") == 0) TestProtectionCountsPerStepAndCanBeDisabled();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
