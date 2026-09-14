#ifndef DAQC_RUNTIME_H
#define DAQC_RUNTIME_H

/* Owns the DAQC transport, XRCE bridge, ROS control, and output lifecycle for one run. */

#include <stdbool.h>

#include "app_config.h"
#include "daq_output_bridge.h"
#include "daq_serial_service.h"
#include "daq_xrce_udp_bridge.h"
#include "daqc_ros_control.h"

typedef struct {
    daq_coordinator_t coordinator;
    daq_xrce_udp_bridge_t xrce_bridge;
    daq_serial_service_t serial_service;
    daqc_ros_control_t ros_control;
    daq_output_bridge_t output_bridge;
    char failure_stage[64];
    bool coordinator_initialized;
    bool xrce_bridge_started;
    bool serial_service_started;
    bool ros_control_initialized;
} daqc_runtime_t;

int DaqcRuntimeStart(daqc_runtime_t *runtime, const AppConfig *config, input_state_t *input_state);
void DaqcRuntimeStop(daqc_runtime_t *runtime, const AppConfig *config);
bool DaqcRuntimeGetSerialStats(daqc_runtime_t *runtime, daq_serial_service_stats_t *stats);
const char *DaqcRuntimeFailureStage(const daqc_runtime_t *runtime);
void DaqcRuntimeReportDiagnostics(const daqc_runtime_t *runtime);

#endif
