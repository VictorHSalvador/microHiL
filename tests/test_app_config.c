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
