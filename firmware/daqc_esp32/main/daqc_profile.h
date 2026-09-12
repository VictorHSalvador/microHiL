#ifndef DAQC_PROFILE_H
#define DAQC_PROFILE_H

/* Contrato público do módulo daqc profile. */

#include <stdbool.h>
#include <stdint.h>

#define DAQC_PROFILE_ESP32 1U
#define DAQC_ACQUISITION_MAX_FREQUENCY_HZ 100U

typedef struct {
    uint8_t adc_resolution_bits;
    uint8_t adc_attenuation[6];
    uint32_t pwm_frequency_hz[2];
    uint8_t pwm_resolution_bits[2];
    uint16_t acquisition_frequency_hz;
} daqc_configuration_t;

bool DaqcProfileConfigure(const daqc_configuration_t *configuration);
bool DaqcProfileApplyActuation(const uint8_t payload[21]);
bool DaqcProfileAcquire(uint8_t payload[28]);
uint32_t DaqcProfileAcquisitionPeriodUs(void);
void DaqcProfileSetSafeOutputs(void);

#endif
