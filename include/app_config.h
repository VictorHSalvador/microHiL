#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Contrato público do módulo app config. */

#include <stdbool.h>
#include <stddef.h>
#include "common.h"
#include "input_state.h"
#include "profile_config.h"

typedef struct {
    char fmu_path[PATH_LEN];
    double step_size_s;
    double stop_time_s;
    int rt_priority;
    int cpu_core;
    bool strict_realtime;
    bool binary_log_enabled;
    char binary_log_path[PATH_LEN];
    bool plot_enabled;
    double plot_window_s;
    double plot_refresh_s;
    OutputVariable outputs[MAX_OUTPUTS];
    size_t output_count;
    input_channel_descriptor_t inputs[INPUT_STATE_MAX_CHANNELS];
    size_t input_count;
    bool stop_on_invalid_input_limit;
    uint32_t daqc_config_timeout_ms;
    uint32_t daqc_ros_startup_delay_ms;
    uint32_t daqc_ros_timeout_ms;
    bool daqc_enabled;
    char daqc_device_path[PATH_LEN];
    unsigned int daqc_baud_rate;
    uint16_t daqc_agent_port;
    char profile_path[PATH_LEN];
    profile_config_t profile;
    daq_schema_t acquisition_schema;
    daq_schema_t actuation_schema;
    bool profile_loaded;
} AppConfig;

void app_config_set_defaults(AppConfig *config);
void app_config_print(const AppConfig *config);
int app_config_normalize_outputs(AppConfig *config);
bool app_config_timing_is_valid(const AppConfig *config);

#endif
