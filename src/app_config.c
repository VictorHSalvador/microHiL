#include "app_config.h"
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
}

void app_config_print(const AppConfig *config) {
    printf("\n=== Current configuration ===\n");
    printf("FMU:                 %s\n", config->fmu_path[0] ? config->fmu_path : "<not loaded>");
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
