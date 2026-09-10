#ifndef DAQC_CONTROL_H
#define DAQC_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "daqc_profile.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    SemaphoreHandle_t mutex;
    volatile uint8_t state;
    volatile bool profile_applied;
    volatile bool configuration_applied;
    bool initialized;
} daqc_control_t;

typedef enum {
    DAQC_CONTROL_OK = 0,
    DAQC_CONTROL_INVALID_COMMAND,
    DAQC_CONTROL_BUSY,
    DAQC_CONTROL_PROFILE,
    DAQC_CONTROL_STATE_MISMATCH,
    DAQC_CONTROL_STREAMING,
    DAQC_CONTROL_CONFIGURATION
} daqc_control_status_t;

bool DaqcControlInit(daqc_control_t *control);
uint8_t DaqcControlState(const daqc_control_t *control);
bool DaqcControlProfileApplied(const daqc_control_t *control);
bool DaqcControlConfigurationApplied(const daqc_control_t *control);
daqc_control_status_t DaqcControlApplyCommand(daqc_control_t *control, uint8_t command);
daqc_control_status_t DaqcControlApplySetup(daqc_control_t *control, uint8_t command, uint32_t profile_id,
                                             bool apply_configuration, const daqc_configuration_t *configuration);

#endif
