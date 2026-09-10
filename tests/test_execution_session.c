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

int main(int argc, char **argv) {
    Require(argc == 2, "expected the fixture FMU path");
    execution_session_t session;
    OutputVariable outputs[MAX_OUTPUTS];

    ExecutionSessionInit(&session);
    Require(ExecutionSessionLoadFmu(&session, argv[1]) == EXECUTION_SESSION_OK, "could not load the fixture FMU into the session");
    Require(fmu_model_name(&session.model)[0] != '\0', "session did not preserve the FMU model name");
    const size_t output_count = ExecutionSessionListOutputs(&session, outputs, MAX_OUTPUTS);
    Require(output_count == 3U, "session did not discover the numeric FMU outputs");
    memcpy(session.config.outputs, outputs, output_count * sizeof(outputs[0]));
    session.config.output_count = output_count;
    session.config.binary_log_enabled = false;
    Require(ExecutionSessionSetTiming(&session, 0.01, 0.02) == EXECUTION_SESSION_OK, "could not configure session timing");
    Require(ExecutionSessionPrepare(&session) == EXECUTION_SESSION_OK, "could not prepare the session FMU");
    Require(ExecutionSessionStartLogging(&session) == EXECUTION_SESSION_OK, "disabled logging should not block the session");
    Require(ExecutionSessionStart(&session, NULL, NULL) == EXECUTION_SESSION_OK, "could not start the session simulation");
    Require(ExecutionSessionJoin(&session) == EXECUTION_SESSION_OK, "could not complete the session simulation");
    const run_result_t *result = ExecutionSessionResult(&session);
    Require(result && result->simulation.state == SIMULATION_RUN_FINISHED && result->simulation.stats.completed_steps == 2U,
            "session result did not report the fixed simulation steps");
    ExecutionSessionDestroy(&session);
    return EXIT_SUCCESS;
}
