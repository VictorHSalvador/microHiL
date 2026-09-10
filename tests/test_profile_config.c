#include "profile_config.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    profile_config_t config;
    if (ProfileConfigLoadYaml(argv[1], &config) != PROFILE_CONFIG_OK) return 1;
    if (config.version != 1U || config.profile_id != 1U || config.mapping_count != 2U) return 1;
    if (strcmp(config.mappings[0].channel, "GPIO32_AI") != 0 || strcmp(config.mappings[0].variable, "u_real") != 0 ||
        config.mappings[0].type != NUMERIC_REAL || config.mappings[1].type != NUMERIC_BOOLEAN) return 1;
    return 0;
}
