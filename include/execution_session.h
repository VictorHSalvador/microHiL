#ifndef EXECUTION_SESSION_H
#define EXECUTION_SESSION_H

#include "app_config.h"
#include "fmu_model.h"

typedef struct {
    FmuModel model;
    AppConfig config;
    bool initialized;
} execution_session_t;

typedef enum {
    EXECUTION_SESSION_OK = 0,
    EXECUTION_SESSION_INVALID_ARGUMENT,
    EXECUTION_SESSION_FMU_LOAD,
    EXECUTION_SESSION_INPUT_LIMIT,
    EXECUTION_SESSION_TIMING
} execution_session_status_t;

void ExecutionSessionInit(execution_session_t *session);
void ExecutionSessionDestroy(execution_session_t *session);
execution_session_status_t ExecutionSessionLoadFmu(execution_session_t *session, const char *path);
execution_session_status_t ExecutionSessionSetTiming(execution_session_t *session, double step_size_s, double stop_time_s);
size_t ExecutionSessionListOutputs(execution_session_t *session, OutputVariable *outputs, size_t capacity);
const char *ExecutionSessionStatusString(execution_session_status_t status);

#endif
