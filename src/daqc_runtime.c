/* Implements the DAQC lifecycle shared by terminal and GUI execution paths. */
#define _POSIX_C_SOURCE 200809L
#include "daqc_runtime.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "daq_state_control.h"

static int WaitDaqcRosStartup(uint32_t delay_ms) {
    struct timespec remaining = {.tv_sec = (time_t)(delay_ms / 1000U), .tv_nsec = (long)(delay_ms % 1000U) * 1000000L};
    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno != EINTR) return -1;
    }
    return 0;
}

static void SetFailureStage(daqc_runtime_t *runtime, const char *failure_stage) {
    snprintf(runtime->failure_stage, sizeof(runtime->failure_stage), "%s", failure_stage);
}

void DaqcRuntimeReportDiagnostics(const daqc_runtime_t *runtime) {
    if (!runtime) return;
    daq_serial_service_stats_t serial_stats = {0};
    if (runtime->serial_service_started && DaqSerialServiceGetStats((daq_serial_service_t *)&runtime->serial_service, &serial_stats)) {
        fprintf(stderr, "DAQC preflight serial: rx=%llu tx=%llu timeouts=%llu failures=%llu.\n", (unsigned long long)serial_stats.received_bytes,
                (unsigned long long)serial_stats.transmitted_frames, (unsigned long long)serial_stats.read_timeouts,
                (unsigned long long)serial_stats.io_failures);
    }
    if (runtime->xrce_bridge_started) {
        fprintf(stderr, "DAQC preflight XRCE: daqc_to_agent=%llu agent_to_daqc=%llu rejected=%llu received=%llu queue_overflows=%llu rejected_frames=%llu.\n",
                (unsigned long long)atomic_load_explicit(&runtime->xrce_bridge.forwarded_to_agent, memory_order_relaxed),
                (unsigned long long)atomic_load_explicit(&runtime->xrce_bridge.forwarded_to_daqc, memory_order_relaxed),
                (unsigned long long)atomic_load_explicit(&runtime->xrce_bridge.rejected_datagrams, memory_order_relaxed),
                (unsigned long long)runtime->coordinator.xrce_frames, (unsigned long long)runtime->coordinator.xrce_queue_overflows,
                (unsigned long long)runtime->coordinator.rejected_frames);
    }
}

void DaqcRuntimeStop(daqc_runtime_t *runtime, const AppConfig *config) {
    if (!runtime) return;
    if (runtime->coordinator_initialized && runtime->serial_service_started && config) {
        daq_link_mode_t mode;
        if (DaqCoordinatorGetMode(&runtime->coordinator, &mode) == DAQ_COORDINATOR_OK && mode != DAQ_LINK_DISABLED) {
            (void)DaqStateControlTransition(&runtime->coordinator, mode, DAQ_PROTOCOL_COMMAND_DISABLE, config->daqc_config_timeout_ms);
        }
    }
    if (runtime->serial_service_started) DaqSerialServiceStop(&runtime->serial_service);
    if (runtime->ros_control_initialized) DaqcRosControlStop(&runtime->ros_control);
    if (runtime->xrce_bridge_started) DaqXrceUdpBridgeStop(&runtime->xrce_bridge);
    if (runtime->coordinator_initialized) DaqCoordinatorDestroy(&runtime->coordinator);
    *runtime = (daqc_runtime_t){0};
}

int DaqcRuntimeStart(daqc_runtime_t *runtime, const AppConfig *config, input_state_t *input_state) {
    if (!runtime || !config || !input_state || !config->profile_loaded || !config->daqc_device_path[0]) return -1;
    *runtime = (daqc_runtime_t){0};
    const daq_coordinator_config_t coordinator_config = {
        .acquisition_schema = &config->acquisition_schema,
        .input_state = input_state,
        .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU,
        .xrce_receive = DaqXrceUdpBridgeReceiveFromDaqc,
        .xrce_context = &runtime->xrce_bridge,
    };
    SetFailureStage(runtime, "coordinator initialization");
    if (DaqCoordinatorInit(&runtime->coordinator, &coordinator_config) != DAQ_COORDINATOR_OK) goto fail;
    runtime->coordinator_initialized = true;
    const daq_xrce_udp_bridge_config_t bridge_config = {.coordinator = &runtime->coordinator, .agent_port = config->daqc_agent_port};
    SetFailureStage(runtime, "XRCE UDP bridge startup");
    if (DaqXrceUdpBridgeStart(&runtime->xrce_bridge, &bridge_config) != DAQ_XRCE_UDP_BRIDGE_OK) goto fail;
    runtime->xrce_bridge_started = true;
    const daq_serial_service_config_t serial_config = {
        .coordinator = &runtime->coordinator,
        .device_path = config->daqc_device_path,
        .baud_rate = config->daqc_baud_rate,
        .reset_daqc_before_start = config->daqc_reset_before_start,
    };
    SetFailureStage(runtime, "DAQC serial service startup");
    if (DaqSerialServiceStart(&runtime->serial_service, &serial_config) != DAQ_SERIAL_SERVICE_OK) goto fail;
    runtime->serial_service_started = true;
    SetFailureStage(runtime, "baseline CONFIG DISABLE confirmation");
    if (DaqStateControlConfirmDisable(&runtime->coordinator, config->daqc_config_timeout_ms) != DAQ_STATE_CONTROL_OK) goto fail;
    SetFailureStage(runtime, "ROS control initialization");
    if (DaqcRosControlInit(&runtime->ros_control) != DAQC_ROS_CONTROL_OK) goto fail;
    runtime->ros_control_initialized = true;
    SetFailureStage(runtime, "ROS control startup");
    if (DaqcRosControlStart(&runtime->ros_control) != DAQC_ROS_CONTROL_OK) goto fail;
    SetFailureStage(runtime, "CONFIG DISABLE to ENABLE");
    if (DaqStateControlTransition(&runtime->coordinator, DAQ_LINK_DISABLED, DAQ_PROTOCOL_COMMAND_ENABLE, config->daqc_config_timeout_ms) != DAQ_STATE_CONTROL_OK) goto fail;
    SetFailureStage(runtime, "ROS/XRCE startup delay");
    if (WaitDaqcRosStartup(config->daqc_ros_startup_delay_ms) != 0) goto fail;
    SetFailureStage(runtime, "DaqcSetup publication");
    if (DaqcRosControlPublishSetup(&runtime->ros_control, DAQ_PROTOCOL_COMMAND_ENABLE, &config->profile, true) != DAQC_ROS_CONTROL_OK) goto fail;
    SetFailureStage(runtime, "DaqcState configuration confirmation");
    if (DaqcRosControlWaitConfiguration(&runtime->ros_control, DAQ_PROTOCOL_COMMAND_ENABLE, config->profile.profile_id, config->daqc_ros_timeout_ms) != DAQC_ROS_CONTROL_OK) goto fail;
    SetFailureStage(runtime, "DAQC output bridge initialization");
    if (DaqOutputBridgeInit(&runtime->output_bridge, &config->actuation_schema, &runtime->coordinator) != DAQ_OUTPUT_BRIDGE_OK) goto fail;
    SetFailureStage(runtime, "CONFIG ENABLE to STREAMING");
    if (DaqStateControlPlay(&runtime->coordinator, config->daqc_config_timeout_ms) != DAQ_STATE_CONTROL_OK) goto fail;
    runtime->failure_stage[0] = '\0';
    return 0;

fail:
    return -1;
}

bool DaqcRuntimeGetSerialStats(daqc_runtime_t *runtime, daq_serial_service_stats_t *stats) {
    return runtime && runtime->serial_service_started && stats && DaqSerialServiceGetStats(&runtime->serial_service, stats);
}

const char *DaqcRuntimeFailureStage(const daqc_runtime_t *runtime) {
    return runtime && runtime->failure_stage[0] ? runtime->failure_stage : "initialization";
}
