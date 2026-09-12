/* Implementação do módulo daq state control. */
#define _POSIX_C_SOURCE 200809L
#include "daq_state_control.h"

#include <time.h>

#define DAQ_STATE_CONTROL_RETRY_MS 5U

static uint64_t MonotonicMilliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0U;
    return (uint64_t)now.tv_sec * UINT64_C(1000) + (uint64_t)now.tv_nsec / UINT64_C(1000000);
}

static void WaitOneMillisecond(void) {
    const struct timespec interval = {.tv_sec = 0, .tv_nsec = 1000000L};
    (void)nanosleep(&interval, NULL);
}

static daq_link_mode_t CommandMode(daq_protocol_command_t command) {
    if (command == DAQ_PROTOCOL_COMMAND_ENABLE) return DAQ_LINK_ENABLED;
    if (command == DAQ_PROTOCOL_COMMAND_STREAMING) return DAQ_LINK_STREAMING;
    return DAQ_LINK_DISABLED;
}

daq_state_control_status_t DaqStateControlTransition(daq_coordinator_t *coordinator, daq_link_mode_t expected_mode,
                                                      daq_protocol_command_t target_command, uint32_t timeout_ms) {
    if (!coordinator || timeout_ms == 0U || target_command < DAQ_PROTOCOL_COMMAND_DISABLE || target_command > DAQ_PROTOCOL_COMMAND_STREAMING) {
        return DAQ_STATE_CONTROL_INVALID_ARGUMENT;
    }
    daq_link_mode_t mode;
    if (DaqCoordinatorGetMode(coordinator, &mode) != DAQ_COORDINATOR_OK) return DAQ_STATE_CONTROL_COORDINATOR;
    if (mode != expected_mode) return DAQ_STATE_CONTROL_UNEXPECTED_STATE;

    const uint64_t started_ms = MonotonicMilliseconds();
    const uint64_t deadline_ms = started_ms + timeout_ms;
    uint64_t next_attempt_ms = started_ms;
    const daq_link_mode_t target_mode = CommandMode(target_command);
    while (true) {
        const uint64_t now_ms = MonotonicMilliseconds();
        if (DaqCoordinatorGetMode(coordinator, &mode) != DAQ_COORDINATOR_OK) return DAQ_STATE_CONTROL_COORDINATOR;
        if (mode == target_mode) return DAQ_STATE_CONTROL_OK;
        if (now_ms >= deadline_ms) return DAQ_STATE_CONTROL_TIMEOUT;
        if (now_ms >= next_attempt_ms) {
            const daq_coordinator_status_t queue_status = DaqCoordinatorQueueCommand(coordinator, target_command);
            if (queue_status != DAQ_COORDINATOR_OK && queue_status != DAQ_COORDINATOR_BUSY) return DAQ_STATE_CONTROL_COORDINATOR;
            next_attempt_ms = now_ms + DAQ_STATE_CONTROL_RETRY_MS;
        }
        WaitOneMillisecond();
    }
}

daq_state_control_status_t DaqStateControlPlay(daq_coordinator_t *coordinator, uint32_t timeout_ms) {
    return DaqStateControlTransition(coordinator, DAQ_LINK_ENABLED, DAQ_PROTOCOL_COMMAND_STREAMING, timeout_ms);
}

daq_state_control_status_t DaqStateControlStop(daq_coordinator_t *coordinator, uint32_t timeout_ms) {
    return DaqStateControlTransition(coordinator, DAQ_LINK_STREAMING, DAQ_PROTOCOL_COMMAND_DISABLE, timeout_ms);
}

const char *DaqStateControlStatusString(daq_state_control_status_t status) {
    switch (status) {
        case DAQ_STATE_CONTROL_OK: return "ok";
        case DAQ_STATE_CONTROL_INVALID_ARGUMENT: return "invalid argument";
        case DAQ_STATE_CONTROL_COORDINATOR: return "DAQ coordinator failure";
        case DAQ_STATE_CONTROL_UNEXPECTED_STATE: return "unexpected DAQC state";
        case DAQ_STATE_CONTROL_TIMEOUT: return "DAQC state confirmation timed out";
        default: return "unknown state control status";
    }
}
