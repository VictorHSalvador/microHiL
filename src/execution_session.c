#include "execution_session.h"
#include <stdio.h>
#include <string.h>

void ExecutionSessionInit(execution_session_t *session) {
    if (!session) return;
    *session = (execution_session_t){0};
    app_config_set_defaults(&session->config);
    fmu_model_init(&session->model);
    session->initialized = true;
}

void ExecutionSessionDestroy(execution_session_t *session) {
    if (!session || !session->initialized) return;
    fmu_model_unload(&session->model);
    *session = (execution_session_t){0};
}

execution_session_status_t ExecutionSessionLoadFmu(execution_session_t *session, const char *path) {
    if (!session || !session->initialized || !path || !path[0]) return EXECUTION_SESSION_INVALID_ARGUMENT;
    fmu_model_unload(&session->model);
    if (fmu_model_load(&session->model, path) != 0) return EXECUTION_SESSION_FMU_LOAD;
    session->config.output_count = 0U;
    session->config.profile_loaded = false;
    session->config.profile_path[0] = '\0';
    session->config.input_count = fmu_model_list_numeric_inputs(&session->model, session->config.inputs, INPUT_STATE_MAX_CHANNELS);
    if (session->config.input_count > INPUT_STATE_MAX_CHANNELS) {
        fmu_model_unload(&session->model);
        session->config.input_count = 0U;
        return EXECUTION_SESSION_INPUT_LIMIT;
    }
    snprintf(session->config.fmu_path, sizeof(session->config.fmu_path), "%s", path);
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionSetTiming(execution_session_t *session, double step_size_s, double stop_time_s) {
    if (!session || !session->initialized) return EXECUTION_SESSION_INVALID_ARGUMENT;
    AppConfig candidate = {.step_size_s = step_size_s, .stop_time_s = stop_time_s};
    if (!app_config_timing_is_valid(&candidate)) return EXECUTION_SESSION_TIMING;
    session->config.step_size_s = step_size_s;
    session->config.stop_time_s = stop_time_s;
    return EXECUTION_SESSION_OK;
}

size_t ExecutionSessionListOutputs(execution_session_t *session, OutputVariable *outputs, size_t capacity) {
    if (!session || !session->initialized || !session->model.fmu) return 0U;
    return fmu_model_list_numeric_outputs(&session->model, outputs, capacity);
}

const char *ExecutionSessionStatusString(execution_session_status_t status) {
    switch (status) {
        case EXECUTION_SESSION_OK: return "ok";
        case EXECUTION_SESSION_INVALID_ARGUMENT: return "invalid argument";
        case EXECUTION_SESSION_FMU_LOAD: return "could not load the FMU";
        case EXECUTION_SESSION_INPUT_LIMIT: return "FMU input count exceeds the host limit";
        case EXECUTION_SESSION_TIMING: return "step size and duration must be finite positive values";
        default: return "unknown execution session status";
    }
}
