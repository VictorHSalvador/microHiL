/* Implementação do módulo app config. */
#include "app_config.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void app_config_set_defaults(AppConfig *config) {
    memset(config, 0, sizeof(*config));
    config->step_size_s = 0.02;
    config->stop_time_s = 60.0;
    config->rt_priority = 80;
    config->cpu_core = -1;
    config->strict_realtime = false;
    config->binary_log_enabled = true;
    snprintf(config->binary_log_path, sizeof(config->binary_log_path), "simulation.mhilog");
    config->plot_enabled = true;
    config->plot_window_s = 10.0;
    config->plot_refresh_s = 0.10;
    config->stop_on_invalid_input_limit = true;
    config->daqc_config_timeout_ms = 10U;
    config->daqc_ros_startup_delay_ms = 6000U;
    config->daqc_ros_timeout_ms = 100U;
    config->daqc_reset_before_start = true;
    config->daqc_baud_rate = 152000U;
    config->daqc_agent_port = 8888U;
}

void app_config_print(const AppConfig *config) {
    printf("\n=== Current configuration ===\n");
    printf("FMU:                 %s\n", config->fmu_path[0] ? config->fmu_path : "<not loaded>");
    printf("YAML profile:        %s\n", config->profile_loaded ? config->profile_path : "<not loaded>");
    printf("Step size:           %.9g s (%.3f Hz)\n", config->step_size_s, 1.0 / config->step_size_s);
    printf("Stop time:           %.9g s\n", config->stop_time_s);
    printf("RT policy:           SCHED_FIFO\n");
    printf("RT priority:         %d\n", config->rt_priority);
    printf("CPU affinity:        %s", config->cpu_core >= 0 ? "core " : "disabled");
    if (config->cpu_core >= 0) printf("%d", config->cpu_core);
    printf("\n");
    printf("Strict RT:           %s\n", config->strict_realtime ? "yes" : "no (fallback allowed)");
    printf("Binary logging:      %s", config->binary_log_enabled ? config->binary_log_path : "disabled");
    printf("\n");
    printf("Real-time plot:      %s", config->plot_enabled ? "enabled" : "disabled");
    if (config->plot_enabled) printf(" (window %.2f s, refresh %.3f s)", config->plot_window_s, config->plot_refresh_s);
    printf("\n");
    printf("Selected outputs:    %zu\n", config->output_count);
    for (size_t i = 0; i < config->output_count; ++i) printf("  [XML %u] %s\n", config->outputs[i].xml_index, config->outputs[i].name);
    printf("FMU inputs:          %zu\n", config->input_count);
    if (config->profile_loaded) {
        printf("DAQC profile ID:     %u\n", config->profile.profile_id);
        printf("DAQC mappings:       %zu\n", config->profile.mapping_count);
    }
    printf("Invalid input stop:  %s\n", config->stop_on_invalid_input_limit ? "enabled at 100 steps" : "disabled");
    printf("DAQC CONFIG timeout: %u ms\n", config->daqc_config_timeout_ms);
    printf("DAQC ROS start delay:%u ms\n", config->daqc_ros_startup_delay_ms);
    printf("DAQC ROS timeout:    %u ms\n", config->daqc_ros_timeout_ms);
    printf("DAQC integration:    %s\n", config->daqc_enabled ? "enabled" : "disabled");
    printf("DAQC reset/start:    %s\n", config->daqc_reset_before_start ? "enabled" : "disabled");
    if (config->daqc_enabled) {
        printf("DAQC TTY:            %s\n", config->daqc_device_path[0] ? config->daqc_device_path : "<not configured>");
        printf("DAQC baud rate:      %u bit/s\n", config->daqc_baud_rate);
        printf("DAQC Agent port:     %u\n", (unsigned int)config->daqc_agent_port);
    }
    for (size_t i = 0; i < config->input_count; ++i) {
        printf("  [VR %u] %s\n", config->inputs[i].value_reference, config->inputs[i].input_name);
    }
    printf("=============================\n\n");
}

int app_config_normalize_outputs(AppConfig *config) {
    if (!config || config->output_count > MAX_OUTPUTS) return -1;

    for (size_t index = 1U; index < config->output_count; ++index) {
        OutputVariable selected = config->outputs[index];
        size_t position = index;

        while (position > 0U && config->outputs[position - 1U].xml_index > selected.xml_index) {
            config->outputs[position] = config->outputs[position - 1U];
            --position;
        }
        config->outputs[position] = selected;
    }
    for (size_t index = 1U; index < config->output_count; ++index) {
        if (config->outputs[index - 1U].xml_index == config->outputs[index].xml_index) return -1;
    }
    return 0;
}

bool app_config_timing_is_valid(const AppConfig *config) {
    return config && isfinite(config->step_size_s) && isfinite(config->stop_time_s) && config->step_size_s > 0.0 && config->stop_time_s > 0.0;
}
