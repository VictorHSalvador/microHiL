#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include "common.h"

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
} AppConfig;

void app_config_set_defaults(AppConfig *config);
void app_config_print(const AppConfig *config);
int app_config_normalize_outputs(AppConfig *config);

#endif
