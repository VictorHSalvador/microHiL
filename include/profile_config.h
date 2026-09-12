#ifndef PROFILE_CONFIG_H
#define PROFILE_CONFIG_H

/* Contrato público do módulo profile config. */

#include <stddef.h>
#include <stdint.h>

#include "daq_schema.h"

#define PROFILE_CONFIG_VERSION 1U
#define PROFILE_CONFIG_MAX_MAPPINGS 19U
#define PROFILE_CONFIG_ADC_CHANNELS 6U
#define PROFILE_CONFIG_PWM_CHANNELS 2U
#define PROFILE_CONFIG_DEFAULT_ACQUISITION_FREQUENCY_HZ 100U
#define PROFILE_CONFIG_MAX_ACQUISITION_FREQUENCY_HZ 100U

typedef struct {
    uint8_t adc_resolution_bits;
    uint8_t adc_attenuation[PROFILE_CONFIG_ADC_CHANNELS];
    uint32_t pwm_frequency_hz[PROFILE_CONFIG_PWM_CHANNELS];
    uint8_t pwm_resolution_bits[PROFILE_CONFIG_PWM_CHANNELS];
    uint16_t acquisition_frequency_hz;
} profile_daqc_configuration_t;

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
    profile_daqc_configuration_t daqc_configuration;
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
