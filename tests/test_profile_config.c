#include "profile_config.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 6) return 2;
    profile_config_t config;
    if (ProfileConfigLoadYaml(argv[1], &config) != PROFILE_CONFIG_OK) return 1;
    if (config.version != 1U || config.profile_id != 1U || config.mapping_count != 3U ||
        config.daqc_configuration.acquisition_frequency_hz != 100U) return 1;
    if (strcmp(config.mappings[0].channel, "GPIO32_AI") != 0 || strcmp(config.mappings[0].variable, "u_real") != 0 ||
        config.mappings[0].type != NUMERIC_REAL || config.mappings[1].type != NUMERIC_BOOLEAN ||
        config.daqc_configuration.adc_resolution_bits != 12U || config.daqc_configuration.adc_attenuation[0] != 3U ||
        config.daqc_configuration.pwm_frequency_hz[0] != 20000U || config.daqc_configuration.pwm_resolution_bits[1] != 10U) return 1;
    if (ProfileConfigLoadYaml(argv[2], &config) != PROFILE_CONFIG_OK ||
        config.daqc_configuration.acquisition_frequency_hz != PROFILE_CONFIG_DEFAULT_ACQUISITION_FREQUENCY_HZ) return 1;
    if (ProfileConfigLoadYaml(argv[3], &config) != PROFILE_CONFIG_SCHEMA) return 1;
    if (ProfileConfigLoadYaml(argv[4], &config) != PROFILE_CONFIG_SCHEMA) return 1;
    if (ProfileConfigLoadYaml(argv[5], &config) != PROFILE_CONFIG_SCHEMA) return 1;
    return 0;
}
