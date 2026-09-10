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

execution_session_status_t ExecutionSessionLoadProfile(execution_session_t *session, const char *path) {
    profile_config_t candidate;
    daq_schema_t acquisition_schema;
    daq_schema_t actuation_schema;

    if (!session || !session->initialized || !path || !path[0]) return EXECUTION_SESSION_INVALID_ARGUMENT;
    if (!session->model.fmu) return EXECUTION_SESSION_NOT_READY;
    if (session->prepared || session->run_started) return EXECUTION_SESSION_RUNNING;
    if (ProfileConfigLoadYaml(path, &candidate) != PROFILE_CONFIG_OK || fmu_model_resolve_profile_mappings(&session->model, &candidate) != 0 ||
        ProfileConfigBuildAcquisitionSchema(&candidate, &acquisition_schema) != PROFILE_CONFIG_OK ||
        ProfileConfigBuildActuationSchema(&candidate, &actuation_schema) != PROFILE_CONFIG_OK) return EXECUTION_SESSION_PROFILE;
    session->config.profile = candidate;
    session->config.acquisition_schema = acquisition_schema;
    session->config.actuation_schema = actuation_schema;
    session->config.step_size_s = candidate.step_size_s;
    session->config.stop_time_s = candidate.stop_time_s;
    session->config.profile_loaded = true;
    snprintf(session->config.profile_path, sizeof(session->config.profile_path), "%s", path);
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

static bool GetOutputAt(const execution_session_t *session, size_t index, OutputVariable *output) {
    OutputVariable outputs[MAX_OUTPUTS];
    const size_t count = ExecutionSessionListOutputs((execution_session_t *)session, outputs, MAX_OUTPUTS);
    if (!session || !output || index >= count || index >= MAX_OUTPUTS) return false;
    *output = outputs[index];
    return true;
}

size_t ExecutionSessionOutputCount(const execution_session_t *session) {
    return session ? ExecutionSessionListOutputs((execution_session_t *)session, NULL, 0U) : 0U;
}

const char *ExecutionSessionOutputName(const execution_session_t *session, size_t index) {
    static _Thread_local OutputVariable output;
    return GetOutputAt(session, index, &output) ? output.name : "";
}

unsigned int ExecutionSessionOutputValueReference(const execution_session_t *session, size_t index) {
    OutputVariable output;
    return GetOutputAt(session, index, &output) ? output.value_reference : 0U;
}

int ExecutionSessionOutputType(const execution_session_t *session, size_t index) {
    OutputVariable output;
    return GetOutputAt(session, index, &output) ? (int)output.type : -1;
}

bool ExecutionSessionOutputSelected(const execution_session_t *session, size_t index) {
    OutputVariable output;
    if (!session || !GetOutputAt(session, index, &output)) return false;
    for (size_t selected_index = 0U; selected_index < session->config.output_count; ++selected_index) {
        if (session->config.outputs[selected_index].xml_index == output.xml_index) return true;
    }
    return false;
}

execution_session_status_t ExecutionSessionSetOutputSelected(execution_session_t *session, size_t index, bool selected) {
    OutputVariable output;
    if (!session || !session->initialized || session->prepared || session->run_started || !GetOutputAt(session, index, &output)) return EXECUTION_SESSION_NOT_READY;
    size_t found = session->config.output_count;
    for (size_t selected_index = 0U; selected_index < session->config.output_count; ++selected_index) {
        if (session->config.outputs[selected_index].xml_index == output.xml_index) {
            found = selected_index;
            break;
        }
    }
    if (selected && found == session->config.output_count) {
        if (session->config.output_count == MAX_OUTPUTS) return EXECUTION_SESSION_NOT_READY;
        session->config.outputs[session->config.output_count++] = output;
    } else if (!selected && found < session->config.output_count) {
        memmove(&session->config.outputs[found], &session->config.outputs[found + 1U],
                (session->config.output_count - found - 1U) * sizeof(session->config.outputs[0]));
        --session->config.output_count;
    }
    return EXECUTION_SESSION_OK;
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

const char *ExecutionSessionInputName(const execution_session_t *session, size_t index) {
    return session && session->initialized && index < session->config.input_count ? session->config.inputs[index].input_name : "";
}

unsigned int ExecutionSessionInputValueReference(const execution_session_t *session, size_t index) {
    return session && session->initialized && index < session->config.input_count ? session->config.inputs[index].value_reference : 0U;
}

int ExecutionSessionInputType(const execution_session_t *session, size_t index) {
    return session && session->initialized && index < session->config.input_count ? (int)session->config.inputs[index].type : -1;
}

const char *ExecutionSessionProfilePath(const execution_session_t *session) {
    return session && session->initialized ? session->config.profile_path : "";
}

unsigned int ExecutionSessionProfileId(const execution_session_t *session) {
    return session && session->initialized && session->config.profile_loaded ? session->config.profile.profile_id : 0U;
}

size_t ExecutionSessionProfileMappingCount(const execution_session_t *session) {
    return session && session->initialized && session->config.profile_loaded ? session->config.profile.mapping_count : 0U;
}

double ExecutionSessionStepSize(const execution_session_t *session) {
    return session && session->initialized ? session->config.step_size_s : 0.0;
}

double ExecutionSessionStopTime(const execution_session_t *session) {
    return session && session->initialized ? session->config.stop_time_s : 0.0;
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
        case EXECUTION_SESSION_PROFILE: return "the YAML profile is incompatible with the loaded FMU or ESP32 profile";
        default: return "unknown execution session status";
    }
}
