#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmu_model.h"
#include "profile_config.h"
#include "rt_simulation.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static size_t FindInput(const input_channel_descriptor_t *inputs, size_t count, const char *name) {
    for (size_t index = 0U; index < count; ++index) {
        if (strcmp(inputs[index].input_name, name) == 0) return index;
    }
    return count;
}

static size_t FindOutput(const OutputVariable *outputs, size_t count, const char *name) {
    for (size_t index = 0U; index < count; ++index) {
        if (strcmp(outputs[index].name, name) == 0) return index;
    }
    return count;
}

int main(int argc, char **argv) {
    Require(argc == 3, "expected the fixture FMU and YAML paths");
    FmuModel model;
    AppConfig invalid_realtime_config;
    input_channel_descriptor_t inputs[INPUT_STATE_MAX_CHANNELS];
    input_value_t values[INPUT_STATE_MAX_CHANNELS] = {0};
    OutputVariable outputs[MAX_OUTPUTS];
    log_value_t output_values[MAX_OUTPUTS] = {0};

    fmu_model_init(&model);
    app_config_set_defaults(&invalid_realtime_config);
    invalid_realtime_config.rt_priority = 0;
    char realtime_message[96];
    Require(RtSimulationCheckHilRealtime(&invalid_realtime_config, realtime_message, sizeof(realtime_message)) != 0,
            "HiL real-time preflight must reject an invalid priority");
    Require(fmu_model_load(&model, argv[1]) == 0, "could not load the input fixture FMU");
    const size_t input_count = fmu_model_list_numeric_inputs(&model, inputs, INPUT_STATE_MAX_CHANNELS);
    const size_t output_count = fmu_model_list_numeric_outputs(&model, outputs, MAX_OUTPUTS);
    Require(input_count == 3U && output_count == 3U, "fixture did not expose the expected input/output count");

    const size_t real_input = FindInput(inputs, input_count, "u_real");
    const size_t integer_input = FindInput(inputs, input_count, "u_integer");
    const size_t boolean_input = FindInput(inputs, input_count, "u_boolean");
    const size_t real_output = FindOutput(outputs, output_count, "y_real");
    const size_t boolean_output = FindOutput(outputs, output_count, "y_boolean");
    Require(real_input < input_count && integer_input < input_count && boolean_input < input_count,
            "fixture inputs were not discovered by name");
    Require(real_output < output_count && boolean_output < output_count, "fixture outputs were not discovered by name");
    Require(inputs[real_input].type == NUMERIC_REAL && inputs[integer_input].type == NUMERIC_INTEGER && inputs[boolean_input].type == NUMERIC_BOOLEAN,
            "fixture input types were not preserved");

    profile_config_t profile;
    Require(ProfileConfigLoadYaml(argv[2], &profile) == PROFILE_CONFIG_OK, "could not load the profile fixture");
    Require(fmu_model_resolve_profile_mappings(&model, &profile) == 0 && profile.mappings[0].is_input && !profile.mappings[1].is_input,
            "profile mappings were not resolved against the FMU");
    daq_schema_t acquisition_schema;
    Require(ProfileConfigBuildAcquisitionSchema(&profile, &acquisition_schema) == PROFILE_CONFIG_OK && acquisition_schema.field_count == 1U &&
            acquisition_schema.payload_size == 28U && acquisition_schema.fields[0].gpio == 32U && acquisition_schema.fields[0].offset == 4U,
            "profile acquisition mapping did not produce the ESP32 DATA schema");
    daq_schema_t actuation_schema;
    Require(ProfileConfigBuildActuationSchema(&profile, &actuation_schema) == PROFILE_CONFIG_OK && actuation_schema.field_count == 2U &&
            actuation_schema.payload_size == 21U && actuation_schema.fields[0].gpio == 16U && actuation_schema.fields[1].gpio == 25U,
            "profile actuation mapping did not produce the ESP32 DATA schema");
    Require(fmu_model_initialize_cosimulation(&model, 0.0, 0.1) == 0, "could not initialize the fixture FMU");
    Require(fmu_model_resolve_input_initial_values(&model, inputs, input_count) == 0, "could not resolve FMU input references");
    values[real_input].real_value = 2.5;
    values[integer_input].discrete_value = 17;
    values[boolean_input].boolean_value = 1U;
    Require(fmu_model_set_inputs(&model, inputs, values, input_count) == 0, "could not apply typed FMU inputs");
    Require(fmu_model_resolve_input_initial_values(&model, inputs, input_count) == 0, "could not read back typed FMU inputs");
    Require(fabs(inputs[real_input].initial_value.real_value - 2.5) < 1e-12 && inputs[integer_input].initial_value.discrete_value == 17 &&
            inputs[boolean_input].initial_value.boolean_value == 1U, "FMU did not retain the typed input values");
    Require(fmu_model_do_step(&model, 0.0, 0.1) == 0, "fixture FMU step failed");
    Require(fmu_model_read_outputs(&model, outputs, output_count, output_values) == 0, "could not read fixture outputs");
    Require(fabs(output_values[real_output].real_value - 5.0) < 1e-12, "real input was not applied before the FMU step");
    Require(output_values[boolean_output].boolean_value == 0U, "boolean input was not applied before the FMU step");

    fmu_model_unload(&model);

    AppConfig config;
    app_config_set_defaults(&config);
    config.step_size_s = 0.01;
    config.stop_time_s = 0.02;
    config.input_count = input_count;
    config.output_count = output_count;
    memcpy(config.inputs, inputs, input_count * sizeof(inputs[0]));
    memcpy(config.outputs, outputs, output_count * sizeof(outputs[0]));

    RtSimulationContext simulation;
    _Atomic bool stop_requested = false;
    _Atomic bool producer_done = false;
    fmu_model_init(&model);
    Require(fmu_model_load(&model, argv[1]) == 0, "could not reload the FMU for the preparation test");
    Require(RtSimulationPrepare(&simulation, &model, &config) == 0, "could not prepare the FMU before starting the simulation thread");
    Require(simulation.input_state_ready && simulation.prepared && simulation.stats.completed_steps == 0U,
            "preparation did not leave initialized inputs without executing a simulation step");
    Require(RtSimulationStart(&simulation, NULL, NULL, NULL, &stop_requested, &producer_done, NULL) == 0,
            "could not start a prepared simulation");
    Require(RtSimulationJoin(&simulation) == 0 && simulation.run_result.stats.completed_steps == 2U,
            "prepared simulation did not execute the requested fixed steps");
    Require(atomic_load_explicit(&producer_done, memory_order_acquire), "prepared simulation did not complete its producer state");
    fmu_model_unload(&model);
    return EXIT_SUCCESS;
}
