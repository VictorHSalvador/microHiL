#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "execution_session.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static size_t FindInput(const execution_session_t *session, const char *name) {
    for (size_t index = 0U; index < ExecutionSessionInputCount(session); ++index) {
        if (strcmp(ExecutionSessionInputName(session, index), name) == 0) return index;
    }
    return ExecutionSessionInputCount(session);
}

int main(int argc, char **argv) {
    Require(argc == 3, "expected the fixture FMU and profile paths");
    execution_session_t session;
    OutputVariable outputs[MAX_OUTPUTS];

    ExecutionSessionInit(&session);
    Require(ExecutionSessionLoadFmu(&session, argv[1]) == EXECUTION_SESSION_OK, "could not load the fixture FMU into the session");
    Require(fmu_model_name(&session.model)[0] != '\0', "session did not preserve the FMU model name");
    const size_t real_input = FindInput(&session, "u_real");
    const size_t integer_input = FindInput(&session, "u_integer");
    Require(real_input < ExecutionSessionInputCount(&session) && integer_input < ExecutionSessionInputCount(&session), "session did not expose the fixture inputs");
    Require(ExecutionSessionSetVirtualInput(&session, integer_input, 23.0) == EXECUTION_SESSION_OK, "could not set an integer virtual input");
    Require(ExecutionSessionSetVirtualInput(&session, integer_input, 23.5) == EXECUTION_SESSION_INPUT_VALUE, "session accepted a fractional integer virtual input");
    Require(ExecutionSessionLoadProfile(&session, argv[2]) == EXECUTION_SESSION_OK, "could not load the compatible YAML profile into the session");
    Require(session.config.profile_loaded && session.config.profile.mapping_count == 3U, "session did not retain the resolved YAML profile");
    Require(ExecutionSessionInputHasPhysicalMapping(&session, real_input), "session did not identify the physical input mapping");
    Require(ExecutionSessionSetVirtualInput(&session, real_input, 2.0) == EXECUTION_SESSION_INPUT_PHYSICAL, "session allowed virtual input over a physical mapping");
    const size_t output_count = ExecutionSessionListOutputs(&session, outputs, MAX_OUTPUTS);
    Require(output_count == 3U, "session did not discover the numeric FMU outputs");
    Require(ExecutionSessionOutputCount(&session) == output_count, "session did not expose the output count for the UI");
    Require(!ExecutionSessionOutputSelected(&session, 0U), "session selected an output before a user selection");
    Require(ExecutionSessionSetOutputSelected(&session, 0U, true) == EXECUTION_SESSION_OK, "could not select an FMU output");
    Require(ExecutionSessionOutputSelected(&session, 0U), "session did not retain the output selection");
    Require(ExecutionSessionSetOutputSelected(&session, 0U, false) == EXECUTION_SESSION_OK, "could not remove an FMU output selection");
    Require(!ExecutionSessionOutputSelected(&session, 0U), "session retained a removed output selection");
    for (size_t output_index = 0U; output_index < output_count; ++output_index) {
        Require(ExecutionSessionSetOutputSelected(&session, output_index, true) == EXECUTION_SESSION_OK, "could not select all FMU outputs");
    }
    Require(ExecutionSessionStartGui(&session, 0.01, 0.02, false, true) == EXECUTION_SESSION_OK, "could not start the GUI simulation");
    Require(ExecutionSessionJoinGui(&session) == EXECUTION_SESSION_OK, "could not complete the GUI simulation");
    SimulationSample plot_sample;
    Require(ExecutionSessionPollGuiSample(&session, &plot_sample) && plot_sample.value_count == output_count,
            "GUI session did not publish an output sample");
    const run_result_t *result = ExecutionSessionResult(&session);
    Require(result && result->simulation.state == SIMULATION_RUN_FINISHED && result->simulation.stats.completed_steps == 2U,
            "session result did not report the fixed simulation steps");
    ExecutionSessionDestroy(&session);
    return EXIT_SUCCESS;
}
