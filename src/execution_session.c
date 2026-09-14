/* Implementação do módulo execution session. */
#include "execution_session.h"
#include "log_converter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static void ClearRunState(execution_session_t *session) {
    session->simulation = (RtSimulationContext){0};
    session->logging = (run_logging_t){0};
    session->result = (run_result_t){0};
    atomic_store_explicit(&session->stop_requested, false, memory_order_relaxed);
    atomic_store_explicit(&session->producer_done, false, memory_order_relaxed);
    atomic_store_explicit(&session->gui_plot_enabled, false, memory_order_relaxed);
    atomic_store_explicit(&session->hil_run_active, false, memory_order_relaxed);
    session->prepared = false;
    session->logging_started = false;
    session->run_started = false;
    session->result_available = false;
#ifdef MICROHIL_WITH_ROS2_CONTROL
    session->daqc_stats = (daq_serial_service_stats_t){0};
    session->daqc_stats_available = false;
#endif
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
    memset(session->virtual_input_values, 0, sizeof(session->virtual_input_values));
    memset(session->virtual_input_set, 0, sizeof(session->virtual_input_set));
    session->config.input_count = fmu_model_list_numeric_inputs(&session->model, session->config.inputs, INPUT_STATE_MAX_CHANNELS);
    if (session->config.input_count > INPUT_STATE_MAX_CHANNELS) {
        fmu_model_unload(&session->model);
        session->config.input_count = 0U;
        return EXECUTION_SESSION_INPUT_LIMIT;
    }
    snprintf(session->config.fmu_path, sizeof(session->config.fmu_path), "%s", path);
    return EXECUTION_SESSION_OK;
}

static execution_session_status_t LoadProfileCandidate(execution_session_t *session, const char *path, profile_config_t *candidate,
                                                        daq_schema_t *acquisition_schema, daq_schema_t *actuation_schema) {
    if (!session || !session->initialized || !path || !path[0]) return EXECUTION_SESSION_INVALID_ARGUMENT;
    if (!session->model.fmu) return EXECUTION_SESSION_NOT_READY;
    if (session->prepared || session->run_started) return EXECUTION_SESSION_RUNNING;
    if (ProfileConfigLoadYaml(path, candidate) != PROFILE_CONFIG_OK || fmu_model_resolve_profile_mappings(&session->model, candidate) != 0 ||
        ProfileConfigBuildAcquisitionSchema(candidate, acquisition_schema) != PROFILE_CONFIG_OK ||
        ProfileConfigBuildActuationSchema(candidate, actuation_schema) != PROFILE_CONFIG_OK) return EXECUTION_SESSION_PROFILE;
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionValidateProfile(execution_session_t *session, const char *path) {
    profile_config_t candidate;
    daq_schema_t acquisition_schema;
    daq_schema_t actuation_schema;
    return LoadProfileCandidate(session, path, &candidate, &acquisition_schema, &actuation_schema);
}

execution_session_status_t ExecutionSessionLoadProfile(execution_session_t *session, const char *path) {
    profile_config_t candidate;
    daq_schema_t acquisition_schema;
    daq_schema_t actuation_schema;

    const execution_session_status_t status = LoadProfileCandidate(session, path, &candidate, &acquisition_schema, &actuation_schema);
    if (status != EXECUTION_SESSION_OK) return status;
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

bool ExecutionSessionInputHasPhysicalMapping(const execution_session_t *session, size_t index) {
    if (!session || !session->initialized || index >= session->config.input_count || !session->config.profile_loaded) return false;
    for (size_t mapping_index = 0U; mapping_index < session->config.profile.mapping_count; ++mapping_index) {
        const profile_mapping_t *mapping = &session->config.profile.mappings[mapping_index];
        if (mapping->is_input && mapping->fmu_index == index) return true;
    }
    return false;
}

static bool ConvertVirtualInput(NumericType type, double value, input_value_t *converted) {
    if (!converted || !isfinite(value)) return false;
    switch (type) {
        case NUMERIC_REAL:
            converted->real_value = value;
            return true;
        case NUMERIC_INTEGER:
        case NUMERIC_ENUMERATION:
            if (value < (double)INT32_MIN || value > (double)INT32_MAX || trunc(value) != value) return false;
            converted->discrete_value = (int64_t)value;
            return true;
        case NUMERIC_BOOLEAN:
            if (value != 0.0 && value != 1.0) return false;
            converted->boolean_value = (uint8_t)value;
            return true;
        default:
            return false;
    }
}

execution_session_status_t ExecutionSessionSetVirtualInput(execution_session_t *session, size_t index, double value) {
    if (!session || !session->initialized || index >= session->config.input_count) return EXECUTION_SESSION_INVALID_ARGUMENT;
    if (ExecutionSessionInputHasPhysicalMapping(session, index)) return EXECUTION_SESSION_INPUT_PHYSICAL;
    input_value_t converted;
    if (!ConvertVirtualInput(session->config.inputs[index].type, value, &converted)) return EXECUTION_SESSION_INPUT_VALUE;
    session->virtual_input_values[index] = converted;
    session->virtual_input_set[index] = true;
    if (session->prepared && session->simulation.input_state_ready &&
        InputStatePublish(&session->simulation.input_state, index, converted, true) != INPUT_STATE_STATUS_OK) return EXECUTION_SESSION_INPUT_VALUE;
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionSetStopOnInvalidInputLimit(execution_session_t *session, bool enabled) {
    if (!session || !session->initialized) return EXECUTION_SESSION_INVALID_ARGUMENT;
    if (session->prepared || session->run_started) return EXECUTION_SESSION_RUNNING;
    session->config.stop_on_invalid_input_limit = enabled;
    return EXECUTION_SESSION_OK;
}

bool ExecutionSessionStopOnInvalidInputLimit(const execution_session_t *session) {
    return session && session->initialized && session->config.stop_on_invalid_input_limit;
}

execution_session_status_t ExecutionSessionStartGui(execution_session_t *session, double step_size_s, double stop_time_s, bool logging_enabled, bool plot_enabled) {
    if (!session || !session->initialized) return EXECUTION_SESSION_INVALID_ARGUMENT;
    session->result = (run_result_t){0};
    session->result_available = false;
#ifdef MICROHIL_WITH_ROS2_CONTROL
    session->config.daqc_enabled = false;
    session->daqc_stats = (daq_serial_service_stats_t){0};
    session->daqc_stats_available = false;
#endif
    execution_session_status_t status = ExecutionSessionSetTiming(session, step_size_s, stop_time_s);
    if (status != EXECUTION_SESSION_OK) return status;
    session->config.binary_log_enabled = logging_enabled;
    // The GUI controls publication through an atomic flag while the run is active.
    session->config.plot_enabled = true;
    atomic_store_explicit(&session->gui_plot_enabled, plot_enabled, memory_order_relaxed);
    if ((status = ExecutionSessionPrepare(session)) != EXECUTION_SESSION_OK) return status;
    if ((status = ExecutionSessionStartLogging(session)) != EXECUTION_SESSION_OK) {
        ExecutionSessionAbort(session);
        return status;
    }
    sample_queue_init(&session->gui_plot_queue);
    if (RtSimulationStart(&session->simulation, session->logging_started ? &session->logging : NULL, &session->gui_plot_queue, NULL,
                          &session->stop_requested, &session->producer_done, &session->gui_plot_enabled) != 0) {
        ExecutionSessionAbort(session);
        return EXECUTION_SESSION_START;
    }
    session->run_started = true;
    return EXECUTION_SESSION_OK;
}

#ifdef MICROHIL_WITH_ROS2_CONTROL
static bool ProfileOutputsAreSelected(const execution_session_t *session) {
    for (size_t mapping_index = 0U; mapping_index < session->config.profile.mapping_count; ++mapping_index) {
        const profile_mapping_t *mapping = &session->config.profile.mappings[mapping_index];
        if (mapping->is_input) continue;
        bool selected = false;
        for (size_t output_index = 0U; output_index < session->config.output_count; ++output_index) {
            const OutputVariable *output = &session->config.outputs[output_index];
            if (output->fmu_index == mapping->fmu_index && output->value_reference == mapping->value_reference && output->type == mapping->type) {
                selected = true;
                break;
            }
        }
        if (!selected) return false;
    }
    return true;
}
#endif

execution_session_status_t ExecutionSessionRunGuiHil(execution_session_t *session, double step_size_s, double stop_time_s, bool logging_enabled,
                                                      bool plot_enabled, const char *device_path) {
#ifndef MICROHIL_WITH_ROS2_CONTROL
    (void)session;
    (void)step_size_s;
    (void)stop_time_s;
    (void)logging_enabled;
    (void)plot_enabled;
    (void)device_path;
    return EXECUTION_SESSION_NOT_SUPPORTED;
#else
    if (!session || !session->initialized || !device_path || !device_path[0]) return EXECUTION_SESSION_INVALID_ARGUMENT;
    session->result = (run_result_t){0};
    session->result_available = false;
    if (!session->config.profile_loaded || !ProfileOutputsAreSelected(session)) return EXECUTION_SESSION_PROFILE;
    execution_session_status_t status = ExecutionSessionSetTiming(session, step_size_s, stop_time_s);
    if (status != EXECUTION_SESSION_OK) return status;
    session->config.binary_log_enabled = logging_enabled;
    session->config.plot_enabled = true;
    session->config.daqc_enabled = true;
    snprintf(session->config.daqc_device_path, sizeof(session->config.daqc_device_path), "%s", device_path);
    session->daqc_stats = (daq_serial_service_stats_t){0};
    session->daqc_stats_available = false;
    atomic_store_explicit(&session->gui_plot_enabled, plot_enabled, memory_order_relaxed);
    atomic_store_explicit(&session->hil_run_active, true, memory_order_release);
    char realtime_message[160];
    if (RtSimulationCheckHilRealtime(&session->config, realtime_message, sizeof(realtime_message)) != 0) {
        atomic_store_explicit(&session->hil_run_active, false, memory_order_release);
        return EXECUTION_SESSION_REALTIME;
    }
    if ((status = ExecutionSessionPrepare(session)) != EXECUTION_SESSION_OK) goto finish;
    if (DaqcRuntimeStart(&session->daqc_runtime, &session->config, &session->simulation.input_state) != 0) {
        status = EXECUTION_SESSION_DAQC;
        DaqcRuntimeStop(&session->daqc_runtime, &session->config);
        ExecutionSessionAbort(session);
        goto finish;
    }
    if ((status = ExecutionSessionStartLogging(session)) != EXECUTION_SESSION_OK) {
        ExecutionSessionAbort(session);
        goto stop_daqc;
    }
    sample_queue_init(&session->gui_plot_queue);
    if (RtSimulationStart(&session->simulation, session->logging_started ? &session->logging : NULL, &session->gui_plot_queue,
                          &session->daqc_runtime.output_bridge, &session->stop_requested, &session->producer_done, &session->gui_plot_enabled) != 0) {
        status = EXECUTION_SESSION_START;
        ExecutionSessionAbort(session);
        goto stop_daqc;
    }
    session->run_started = true;
    status = ExecutionSessionJoin(session);

stop_daqc:
    session->daqc_stats_available = DaqcRuntimeGetSerialStats(&session->daqc_runtime, &session->daqc_stats);
    DaqcRuntimeStop(&session->daqc_runtime, &session->config);
finish:
    atomic_store_explicit(&session->hil_run_active, false, memory_order_release);
    return status;
#endif
}

execution_session_status_t ExecutionSessionStopGui(execution_session_t *session) {
    if (session && atomic_load_explicit(&session->hil_run_active, memory_order_acquire)) {
        atomic_store_explicit(&session->stop_requested, true, memory_order_relaxed);
        return EXECUTION_SESSION_OK;
    }
    return ExecutionSessionRequestStop(session);
}

execution_session_status_t ExecutionSessionSetGuiPlotEnabled(execution_session_t *session, bool enabled) {
    if (!session || !session->initialized) return EXECUTION_SESSION_INVALID_ARGUMENT;
    atomic_store_explicit(&session->gui_plot_enabled, enabled, memory_order_relaxed);
    return EXECUTION_SESSION_OK;
}

execution_session_status_t ExecutionSessionJoinGui(execution_session_t *session) {
    return ExecutionSessionJoin(session);
}

bool ExecutionSessionPollGuiSample(execution_session_t *session, SimulationSample *sample) {
    return session && sample && sample_queue_pop(&session->gui_plot_queue, sample);
}

bool ExecutionSessionGuiRunning(const execution_session_t *session) {
    return ExecutionSessionIsRunning(session);
}

bool ExecutionSessionHasResult(const execution_session_t *session) {
    return session && session->result_available;
}

int ExecutionSessionResultState(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? (int)session->result.simulation.state : -1;
}

const char *ExecutionSessionResultStage(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.stage : "";
}

const char *ExecutionSessionResultMessage(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.message : "";
}

uint64_t ExecutionSessionCompletedSteps(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.stats.completed_steps : 0U;
}

uint64_t ExecutionSessionDeadlineMisses(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.stats.deadline_misses : 0U;
}

uint64_t ExecutionSessionUnusedReleases(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.stats.unused_releases : 0U;
}

double ExecutionSessionMaxComputation(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.stats.max_computation_s : 0.0;
}

double ExecutionSessionMaxLateness(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) ? session->result.simulation.stats.max_lateness_s : 0.0;
}

bool ExecutionSessionSchedFifoActive(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) && session->result.simulation.stats.sched_fifo_active;
}

bool ExecutionSessionDaqcStatsAvailable(const execution_session_t *session) {
#ifdef MICROHIL_WITH_ROS2_CONTROL
    return session && session->daqc_stats_available;
#else
    (void)session;
    return false;
#endif
}

uint64_t ExecutionSessionDaqcRxBytes(const execution_session_t *session) {
#ifdef MICROHIL_WITH_ROS2_CONTROL
    return ExecutionSessionDaqcStatsAvailable(session) ? session->daqc_stats.received_bytes : 0U;
#else
    (void)session;
    return 0U;
#endif
}

uint64_t ExecutionSessionDaqcTxFrames(const execution_session_t *session) {
#ifdef MICROHIL_WITH_ROS2_CONTROL
    return ExecutionSessionDaqcStatsAvailable(session) ? session->daqc_stats.transmitted_frames : 0U;
#else
    (void)session;
    return 0U;
#endif
}

uint64_t ExecutionSessionDaqcReadTimeouts(const execution_session_t *session) {
#ifdef MICROHIL_WITH_ROS2_CONTROL
    return ExecutionSessionDaqcStatsAvailable(session) ? session->daqc_stats.read_timeouts : 0U;
#else
    (void)session;
    return 0U;
#endif
}

uint64_t ExecutionSessionDaqcIoFailures(const execution_session_t *session) {
#ifdef MICROHIL_WITH_ROS2_CONTROL
    return ExecutionSessionDaqcStatsAvailable(session) ? session->daqc_stats.io_failures : 0U;
#else
    (void)session;
    return 0U;
#endif
}

bool ExecutionSessionHasClosedBinaryLog(const execution_session_t *session) {
    return ExecutionSessionHasResult(session) && session->result.logging.enabled && session->result.logging.status == RUN_LOGGING_STATUS_OK &&
           !session->result.logging.logger.incomplete;
}

const char *ExecutionSessionBinaryLogPath(const execution_session_t *session) {
    return ExecutionSessionHasClosedBinaryLog(session) ? session->config.binary_log_path : "";
}

execution_session_status_t ExecutionSessionExportGuiCsv(execution_session_t *session, const char *csv_path, uint64_t *exported_records, bool *partial) {
    log_descriptor_t descriptor;
    log_converter_result_t result;
    log_converter_status_t status;

    if (exported_records) *exported_records = 0U;
    if (partial) *partial = false;
    if (!session || !session->initialized || !csv_path || !csv_path[0]) return EXECUTION_SESSION_INVALID_ARGUMENT;
    if (!ExecutionSessionHasClosedBinaryLog(session)) return EXECUTION_SESSION_NOT_READY;
    if (RunLoggingBuildDescriptor(session->config.fmu_path, session->config.outputs, session->config.output_count, session->config.step_size_s, &descriptor) !=
        RUN_LOGGING_STATUS_OK) return EXECUTION_SESSION_CSV;
    status = LogConverterConvert(LOG_CONVERTER_EXECUTION_CLOSED, session->config.binary_log_path, csv_path, &descriptor, &result);
    if (exported_records) *exported_records = result.exported_records;
    if (partial) *partial = status == LOG_CONVERTER_STATUS_PARTIAL;
    return status == LOG_CONVERTER_STATUS_OK || status == LOG_CONVERTER_STATUS_PARTIAL ? EXECUTION_SESSION_OK : EXECUTION_SESSION_CSV;
}

size_t ExecutionSessionSelectedOutputCount(const execution_session_t *session) {
    return session && session->initialized ? session->config.output_count : 0U;
}

const char *ExecutionSessionSelectedOutputName(const execution_session_t *session, size_t index) {
    return session && session->initialized && index < session->config.output_count ? session->config.outputs[index].name : "";
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

bool ExecutionSessionProfileMappingAt(const execution_session_t *session, size_t index, profile_mapping_t *mapping) {
    if (!session || !session->initialized || !session->config.profile_loaded || !mapping || index >= session->config.profile.mapping_count) return false;
    *mapping = session->config.profile.mappings[index];
    return true;
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
    for (size_t index = 0U; index < session->config.input_count; ++index) {
        if (session->virtual_input_set[index] &&
            InputStatePublish(&session->simulation.input_state, index, session->virtual_input_values[index], true) != INPUT_STATE_STATUS_OK) {
            RtSimulationAbort(&session->simulation);
            return EXECUTION_SESSION_PREPARE;
        }
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
                          &session->stop_requested, &session->producer_done, NULL) != 0) {
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
        case EXECUTION_SESSION_INPUT_PHYSICAL: return "the FMU input is mapped to the physical DAQC";
        case EXECUTION_SESSION_INPUT_VALUE: return "the virtual input value is invalid";
        case EXECUTION_SESSION_CSV: return "could not export the binary log to CSV";
        case EXECUTION_SESSION_REALTIME: return "SCHED_FIFO is unavailable for Play HiL";
        case EXECUTION_SESSION_DAQC: return "could not prepare or stop the DAQC lifecycle";
        case EXECUTION_SESSION_NOT_SUPPORTED: return "this build does not include ROS 2 DAQC support";
        default: return "unknown execution session status";
    }
}
