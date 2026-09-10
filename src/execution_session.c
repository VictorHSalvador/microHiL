#include "execution_session.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void ClearRunState(execution_session_t *session) {
    session->simulation = (RtSimulationContext){0};
    session->logging = (run_logging_t){0};
    session->result = (run_result_t){0};
    atomic_store_explicit(&session->stop_requested, false, memory_order_relaxed);
    atomic_store_explicit(&session->producer_done, false, memory_order_relaxed);
    session->prepared = false;
    session->logging_started = false;
    session->run_started = false;
    session->result_available = false;
}

void ExecutionSessionInit(execution_session_t *session) {
    if (!session) return;
    *session = (execution_session_t){0};
    app_config_set_defaults(&session->config);
    fmu_model_init(&session->model);
    ClearRunState(session);
    session->initialized = true;
}

execution_session_t *ExecutionSessionCreate(void) {
    execution_session_t *session = calloc(1U, sizeof(*session));
    if (session) ExecutionSessionInit(session);
    return session;
}

void ExecutionSessionDestroy(execution_session_t *session) {
    if (!session || !session->initialized) return;
    ExecutionSessionAbort(session);
    fmu_model_unload(&session->model);
    *session = (execution_session_t){0};
}

void ExecutionSessionDelete(execution_session_t *session) {
    if (!session) return;
    ExecutionSessionDestroy(session);
    free(session);
}

execution_session_status_t ExecutionSessionLoadFmu(execution_session_t *session, const char *path) {
    if (!session || !session->initialized || !path || !path[0]) return EXECUTION_SESSION_INVALID_ARGUMENT;
    if (session->run_started) return EXECUTION_SESSION_RUNNING;
    ExecutionSessionAbort(session);
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
    if (session->prepared || session->run_started) return EXECUTION_SESSION_RUNNING;
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

const char *ExecutionSessionFmuPath(const execution_session_t *session) {
    return session && session->initialized ? session->config.fmu_path : "";
}

const char *ExecutionSessionModelName(const execution_session_t *session) {
    return session && session->initialized ? fmu_model_name(&session->model) : "";
}

size_t ExecutionSessionInputCount(const execution_session_t *session) {
    return session && session->initialized ? session->config.input_count : 0U;
}

execution_session_status_t ExecutionSessionPrepare(execution_session_t *session) {
    if (!session || !session->initialized || !session->model.fmu) return EXECUTION_SESSION_NOT_READY;
    if (!app_config_timing_is_valid(&session->config) || session->config.output_count == 0U) return EXECUTION_SESSION_NOT_READY;
    if (session->prepared || session->run_started) return EXECUTION_SESSION_RUNNING;
    if (app_config_normalize_outputs(&session->config) != 0 || RtSimulationPrepare(&session->simulation, &session->model, &session->config) != 0) {
        return EXECUTION_SESSION_PREPARE;
    }
    session->prepared = true;
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionStartLogging(execution_session_t *session) {
    log_descriptor_t descriptor;
    run_logging_status_t status;

    if (!session || !session->prepared || session->run_started) return EXECUTION_SESSION_NOT_READY;
    RunLoggingInit(&session->logging, session->config.binary_log_enabled);
    if (!session->config.binary_log_enabled) return EXECUTION_SESSION_OK;
    status = RunLoggingBuildDescriptor(session->config.fmu_path, session->config.outputs, session->config.output_count, session->config.step_size_s, &descriptor);
    if (status == RUN_LOGGING_STATUS_OK) {
        status = RunLoggingStartFile(&session->logging, &descriptor, session->config.binary_log_path);
    } else {
        (void)RunLoggingStart(&session->logging, NULL, NULL);
        session->logging.result.status = status;
        snprintf(session->logging.result.logger.error_message, sizeof(session->logging.result.logger.error_message), "%s", RunLoggingStatusString(status));
    }
    session->logging_started = true;
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionStart(execution_session_t *session, SampleQueue *plot_queue, daq_output_bridge_t *output_bridge) {
    if (!session || !session->prepared || session->run_started) return EXECUTION_SESSION_NOT_READY;
    if (RtSimulationStart(&session->simulation, session->logging_started ? &session->logging : NULL, plot_queue, output_bridge,
                          &session->stop_requested, &session->producer_done) != 0) {
        return EXECUTION_SESSION_START;
    }
    session->run_started = true;
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionRequestStop(execution_session_t *session) {
    if (!session || !session->run_started) return EXECUTION_SESSION_NOT_READY;
    atomic_store_explicit(&session->stop_requested, true, memory_order_relaxed);
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionJoin(execution_session_t *session) {
    if (!session || !session->run_started) return EXECUTION_SESSION_NOT_READY;
    if (RtSimulationJoin(&session->simulation) != 0 && session->simulation.run_result.state != SIMULATION_RUN_ERROR) return EXECUTION_SESSION_START;
    session->run_started = false;
    RunLoggingFinish(&session->logging);
    RunResultAggregate(&session->result, &session->simulation.run_result, RunLoggingResult(&session->logging));
    session->prepared = false;
    session->logging_started = false;
    session->result_available = true;
    return session->result.simulation.state == SIMULATION_RUN_ERROR ? EXECUTION_SESSION_START : EXECUTION_SESSION_OK;
}

void ExecutionSessionAbort(execution_session_t *session) {
    if (!session || !session->initialized) return;
    if (session->run_started) {
        (void)ExecutionSessionRequestStop(session);
        (void)ExecutionSessionJoin(session);
    } else if (session->prepared) {
        RtSimulationAbort(&session->simulation);
        RunLoggingFinish(&session->logging);
    }
    ClearRunState(session);
}

bool ExecutionSessionIsRunning(const execution_session_t *session) {
    return session && session->run_started && !atomic_load_explicit(&session->producer_done, memory_order_acquire);
}

const run_result_t *ExecutionSessionResult(const execution_session_t *session) {
    return session && session->result_available ? &session->result : NULL;
}

const char *ExecutionSessionStatusString(execution_session_status_t status) {
    switch (status) {
        case EXECUTION_SESSION_OK: return "ok";
        case EXECUTION_SESSION_INVALID_ARGUMENT: return "invalid argument";
        case EXECUTION_SESSION_FMU_LOAD: return "could not load the FMU";
        case EXECUTION_SESSION_INPUT_LIMIT: return "FMU input count exceeds the host limit";
        case EXECUTION_SESSION_TIMING: return "step size and duration must be finite positive values";
        case EXECUTION_SESSION_NOT_READY: return "the session is not ready to run";
        case EXECUTION_SESSION_PREPARE: return "could not prepare the FMU simulation";
        case EXECUTION_SESSION_LOGGING: return "could not start the binary output log";
        case EXECUTION_SESSION_START: return "could not start or complete the simulation";
        case EXECUTION_SESSION_RUNNING: return "the simulation session is active";
        default: return "unknown execution session status";
    }
}
