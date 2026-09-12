#ifndef DAQ_STATE_CONTROL_H
#define DAQ_STATE_CONTROL_H

/* Contrato público do módulo daq state control. */

#include <stdint.h>

#include "daq_coordinator.h"

typedef enum {
    DAQ_STATE_CONTROL_OK = 0,
    DAQ_STATE_CONTROL_INVALID_ARGUMENT,
    DAQ_STATE_CONTROL_COORDINATOR,
    DAQ_STATE_CONTROL_UNEXPECTED_STATE,
    DAQ_STATE_CONTROL_TIMEOUT
} daq_state_control_status_t;

daq_state_control_status_t DaqStateControlTransition(daq_coordinator_t *coordinator, daq_link_mode_t expected_mode,
                                                      daq_protocol_command_t target_command, uint32_t timeout_ms);
daq_state_control_status_t DaqStateControlPlay(daq_coordinator_t *coordinator, uint32_t timeout_ms);
daq_state_control_status_t DaqStateControlStop(daq_coordinator_t *coordinator, uint32_t timeout_ms);
const char *DaqStateControlStatusString(daq_state_control_status_t status);

#endif
