#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    (void)argv;
    Require(argc == 1, "unexpected argument");
    AppConfig config;
    app_config_set_defaults(&config);
    Require(app_config_timing_is_valid(&config), "default timing must be valid");
    Require(config.daqc_config_timeout_ms == 10U, "default DAQC CONFIG timeout must be 10 ms");
    Require(config.daqc_ros_startup_delay_ms == 6000U, "default DAQC ROS startup delay must be 6000 ms");
    Require(config.daqc_ros_timeout_ms == 100U, "default DAQC ROS timeout must be 100 ms");
    Require(!config.daqc_enabled && config.daqc_baud_rate == 152000U && config.daqc_agent_port == 8888U,
            "DAQC link defaults must preserve the selected transport baseline");
    Require(config.daqc_reset_before_start, "DAQC preflight reset must be enabled by default");
    config.step_size_s = 0.0;
    Require(!app_config_timing_is_valid(&config), "zero step must be rejected");
    config.step_size_s = -0.1;
    Require(!app_config_timing_is_valid(&config), "negative step must be rejected");
    config.step_size_s = 0.1;
    config.stop_time_s = NAN;
    Require(!app_config_timing_is_valid(&config), "NaN duration must be rejected");
    config.stop_time_s = INFINITY;
    Require(!app_config_timing_is_valid(&config), "infinite duration must be rejected");
    config.stop_time_s = 0.05;
    Require(app_config_timing_is_valid(&config), "a valid partial final step must be accepted");
    return EXIT_SUCCESS;
}
