#ifndef EXECUTION_SESSION_H
#define EXECUTION_SESSION_H

#include "fmu_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#include "app_config.h"
#include "rt_simulation.h"
#include "run_result.h"
#include "sample_queue.h"

typedef struct execution_session {
    FmuModel model;
    AppConfig config;
    RtSimulationContext simulation;
    run_logging_t logging;
    run_result_t result;
    _Atomic bool stop_requested;
    _Atomic bool producer_done;
    bool initialized;
    bool prepared;
    bool logging_started;
    bool run_started;
    bool result_available;
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
    EXECUTION_SESSION_PROFILE
} execution_session_status_t;

execution_session_t *ExecutionSessionCreate(void);
void ExecutionSessionInit(execution_session_t *session);
void ExecutionSessionDestroy(execution_session_t *session);
void ExecutionSessionDelete(execution_session_t *session);
execution_session_status_t ExecutionSessionLoadFmu(execution_session_t *session, const char *path);
execution_session_status_t ExecutionSessionLoadProfile(execution_session_t *session, const char *path);
execution_session_status_t ExecutionSessionSetTiming(execution_session_t *session, double step_size_s, double stop_time_s);
size_t ExecutionSessionListOutputs(execution_session_t *session, OutputVariable *outputs, size_t capacity);
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
const char *ExecutionSessionProfilePath(const execution_session_t *session);
unsigned int ExecutionSessionProfileId(const execution_session_t *session);
size_t ExecutionSessionProfileMappingCount(const execution_session_t *session);
double ExecutionSessionStepSize(const execution_session_t *session);
double ExecutionSessionStopTime(const execution_session_t *session);
const char *ExecutionSessionStatusString(execution_session_status_t status);

#ifdef __cplusplus
}
#endif

#endif
