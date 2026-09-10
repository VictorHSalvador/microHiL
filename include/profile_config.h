#ifndef PROFILE_CONFIG_H
#define PROFILE_CONFIG_H

#include <stddef.h>

#include "daq_schema.h"

#define PROFILE_CONFIG_VERSION 1U
#define PROFILE_CONFIG_MAX_MAPPINGS 19U

typedef struct {
    char channel[OUTPUT_NAME_LEN];
    char variable[OUTPUT_NAME_LEN];
    NumericType type;
    double scale;
    double offset;
    uint32_t value_reference;
    uint16_t fmu_index;
    bool is_input;
} profile_mapping_t;

typedef struct {
    unsigned int version;
    unsigned int profile_id;
    double step_size_s;
    double stop_time_s;
    profile_mapping_t mappings[PROFILE_CONFIG_MAX_MAPPINGS];
    size_t mapping_count;
} profile_config_t;

typedef enum {
    PROFILE_CONFIG_OK = 0,
    PROFILE_CONFIG_INVALID_ARGUMENT,
    PROFILE_CONFIG_IO,
    PROFILE_CONFIG_SYNTAX,
    PROFILE_CONFIG_SCHEMA,
    PROFILE_CONFIG_UNSUPPORTED
} profile_config_status_t;

profile_config_status_t ProfileConfigLoadYaml(const char *path, profile_config_t *config);
profile_config_status_t ProfileConfigBuildAcquisitionSchema(const profile_config_t *config, daq_schema_t *schema);
profile_config_status_t ProfileConfigBuildActuationSchema(const profile_config_t *config, daq_schema_t *schema);
const char *ProfileConfigStatusString(profile_config_status_t status);

#endif
