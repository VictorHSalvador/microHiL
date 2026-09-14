#ifndef EXECUTION_SESSION_H
#define EXECUTION_SESSION_H

/* Contrato público do módulo execution session. */

#include <stdint.h>

#include "fmu_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#include "app_config.h"
#include "rt_simulation.h"
#include "run_result.h"
#include "sample_queue.h"
#ifdef MICROHIL_WITH_ROS2_CONTROL
#include "daqc_runtime.h"
#endif

typedef struct execution_session {
    FmuModel model;
    AppConfig config;
    RtSimulationContext simulation;
    run_logging_t logging;
    run_result_t result;
    _Atomic bool stop_requested;
    _Atomic bool producer_done;
    _Atomic bool gui_plot_enabled;
    _Atomic bool hil_run_active;
    bool initialized;
    bool prepared;
    bool logging_started;
    bool run_started;
    bool result_available;
    input_value_t virtual_input_values[INPUT_STATE_MAX_CHANNELS];
    bool virtual_input_set[INPUT_STATE_MAX_CHANNELS];
    SampleQueue gui_plot_queue;
#ifdef MICROHIL_WITH_ROS2_CONTROL
    daqc_runtime_t daqc_runtime;
    daq_serial_service_stats_t daqc_stats;
    bool daqc_stats_available;
#endif
} execution_session_t;
#else
typedef struct execution_session execution_session_t;
#endif

typedef enum {
    EXECUTION_SESSION_OK = 0,
    EXECUTION_SESSION_INVALID_ARGUMENT,
    EXECUTION_SESSION_FMU_LOAD,
    EXECUTION_SESSION_INPUT_LIMIT,
    EXECUTION_SESSION_TIMING,
    EXECUTION_SESSION_NOT_READY,
    EXECUTION_SESSION_PREPARE,
    EXECUTION_SESSION_LOGGING,
    EXECUTION_SESSION_START,
    EXECUTION_SESSION_RUNNING,
    EXECUTION_SESSION_PROFILE,
    EXECUTION_SESSION_INPUT_PHYSICAL,
    EXECUTION_SESSION_INPUT_VALUE,
    EXECUTION_SESSION_CSV,
    EXECUTION_SESSION_REALTIME,
    EXECUTION_SESSION_DAQC,
    EXECUTION_SESSION_NOT_SUPPORTED
} execution_session_status_t;

execution_session_t *ExecutionSessionCreate(void);
void ExecutionSessionInit(execution_session_t *session);
void ExecutionSessionDestroy(execution_session_t *session);
void ExecutionSessionDelete(execution_session_t *session);
execution_session_status_t ExecutionSessionLoadFmu(execution_session_t *session, const char *path);
execution_session_status_t ExecutionSessionValidateProfile(execution_session_t *session, const char *path);
execution_session_status_t ExecutionSessionLoadProfile(execution_session_t *session, const char *path);
execution_session_status_t ExecutionSessionSetTiming(execution_session_t *session, double step_size_s, double stop_time_s);
size_t ExecutionSessionListOutputs(execution_session_t *session, OutputVariable *outputs, size_t capacity);
size_t ExecutionSessionOutputCount(const execution_session_t *session);
const char *ExecutionSessionOutputName(const execution_session_t *session, size_t index);
unsigned int ExecutionSessionOutputValueReference(const execution_session_t *session, size_t index);
int ExecutionSessionOutputType(const execution_session_t *session, size_t index);
bool ExecutionSessionOutputSelected(const execution_session_t *session, size_t index);
execution_session_status_t ExecutionSessionSetOutputSelected(execution_session_t *session, size_t index, bool selected);
#ifndef __cplusplus
execution_session_status_t ExecutionSessionPrepare(execution_session_t *session);
execution_session_status_t ExecutionSessionStartLogging(execution_session_t *session);
execution_session_status_t ExecutionSessionStart(execution_session_t *session, SampleQueue *plot_queue, daq_output_bridge_t *output_bridge);
execution_session_status_t ExecutionSessionRequestStop(execution_session_t *session);
execution_session_status_t ExecutionSessionJoin(execution_session_t *session);
void ExecutionSessionAbort(execution_session_t *session);
bool ExecutionSessionIsRunning(const execution_session_t *session);
const run_result_t *ExecutionSessionResult(const execution_session_t *session);
#endif
const char *ExecutionSessionFmuPath(const execution_session_t *session);
const char *ExecutionSessionModelName(const execution_session_t *session);
size_t ExecutionSessionInputCount(const execution_session_t *session);
const char *ExecutionSessionInputName(const execution_session_t *session, size_t index);
unsigned int ExecutionSessionInputValueReference(const execution_session_t *session, size_t index);
int ExecutionSessionInputType(const execution_session_t *session, size_t index);
bool ExecutionSessionInputHasPhysicalMapping(const execution_session_t *session, size_t index);
execution_session_status_t ExecutionSessionSetVirtualInput(execution_session_t *session, size_t index, double value);
execution_session_status_t ExecutionSessionSetStopOnInvalidInputLimit(execution_session_t *session, bool enabled);
bool ExecutionSessionStopOnInvalidInputLimit(const execution_session_t *session);
execution_session_status_t ExecutionSessionStartGui(execution_session_t *session, double step_size_s, double stop_time_s, bool logging_enabled, bool plot_enabled);
execution_session_status_t ExecutionSessionRunGuiHil(execution_session_t *session, double step_size_s, double stop_time_s, bool logging_enabled,
                                                      bool plot_enabled, const char *device_path);
execution_session_status_t ExecutionSessionStopGui(execution_session_t *session);
execution_session_status_t ExecutionSessionSetGuiPlotEnabled(execution_session_t *session, bool enabled);
execution_session_status_t ExecutionSessionJoinGui(execution_session_t *session);
bool ExecutionSessionPollGuiSample(execution_session_t *session, SimulationSample *sample);
bool ExecutionSessionGuiRunning(const execution_session_t *session);
bool ExecutionSessionHasResult(const execution_session_t *session);
int ExecutionSessionResultState(const execution_session_t *session);
const char *ExecutionSessionResultStage(const execution_session_t *session);
const char *ExecutionSessionResultMessage(const execution_session_t *session);
uint64_t ExecutionSessionCompletedSteps(const execution_session_t *session);
uint64_t ExecutionSessionDeadlineMisses(const execution_session_t *session);
uint64_t ExecutionSessionUnusedReleases(const execution_session_t *session);
double ExecutionSessionMaxComputation(const execution_session_t *session);
double ExecutionSessionMaxLateness(const execution_session_t *session);
bool ExecutionSessionSchedFifoActive(const execution_session_t *session);
bool ExecutionSessionDaqcStatsAvailable(const execution_session_t *session);
uint64_t ExecutionSessionDaqcRxBytes(const execution_session_t *session);
uint64_t ExecutionSessionDaqcTxFrames(const execution_session_t *session);
uint64_t ExecutionSessionDaqcReadTimeouts(const execution_session_t *session);
uint64_t ExecutionSessionDaqcIoFailures(const execution_session_t *session);
bool ExecutionSessionHasClosedBinaryLog(const execution_session_t *session);
const char *ExecutionSessionBinaryLogPath(const execution_session_t *session);
execution_session_status_t ExecutionSessionExportGuiCsv(execution_session_t *session, const char *csv_path, uint64_t *exported_records, bool *partial);
size_t ExecutionSessionSelectedOutputCount(const execution_session_t *session);
const char *ExecutionSessionSelectedOutputName(const execution_session_t *session, size_t index);
const char *ExecutionSessionProfilePath(const execution_session_t *session);
unsigned int ExecutionSessionProfileId(const execution_session_t *session);
size_t ExecutionSessionProfileMappingCount(const execution_session_t *session);
bool ExecutionSessionProfileMappingAt(const execution_session_t *session, size_t index, profile_mapping_t *mapping);
double ExecutionSessionStepSize(const execution_session_t *session);
double ExecutionSessionStopTime(const execution_session_t *session);
const char *ExecutionSessionStatusString(execution_session_status_t status);

#ifdef __cplusplus
}
#endif

#endif
