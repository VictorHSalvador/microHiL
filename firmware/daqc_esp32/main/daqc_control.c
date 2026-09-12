/* Implementação do módulo daqc control. */
#include "daqc_control.h"

#include "daqc_protocol.h"

static bool IsCommandValid(uint8_t command) {
    return command >= DAQC_COMMAND_DISABLE && command <= DAQC_COMMAND_STREAMING;
}

bool DaqcControlInit(daqc_control_t *control) {
    if (!control) return false;
    *control = (daqc_control_t){.state = DAQC_COMMAND_DISABLE};
    control->mutex = xSemaphoreCreateMutex();
    control->initialized = control->mutex != NULL;
    return control->initialized;
}

uint8_t DaqcControlState(const daqc_control_t *control) {
    return control && control->initialized ? control->state : DAQC_COMMAND_DISABLE;
}

bool DaqcControlProfileApplied(const daqc_control_t *control) {
    return control && control->initialized && control->profile_applied;
}

bool DaqcControlConfigurationApplied(const daqc_control_t *control) {
    return control && control->initialized && control->configuration_applied;
}

daqc_control_status_t DaqcControlApplyCommand(daqc_control_t *control, uint8_t command) {
    if (!control || !control->initialized || !IsCommandValid(command)) return DAQC_CONTROL_INVALID_COMMAND;
    if (xSemaphoreTake(control->mutex, 0U) != pdTRUE) return DAQC_CONTROL_BUSY;
    if (command == DAQC_COMMAND_STREAMING && !DaqcProfileAcquire((uint8_t[DAQC_ACQUISITION_SIZE]){0})) {
        (void)xSemaphoreGive(control->mutex);
        return DAQC_CONTROL_CONFIGURATION;
    }
    control->state = command;
    if (command != DAQC_COMMAND_STREAMING) DaqcProfileSetSafeOutputs();
    (void)xSemaphoreGive(control->mutex);
    return DAQC_CONTROL_OK;
}

daqc_control_status_t DaqcControlApplySetup(daqc_control_t *control, uint8_t command, uint32_t profile_id,
                                             bool apply_configuration, const daqc_configuration_t *configuration) {
    if (!control || !control->initialized || !IsCommandValid(command)) return DAQC_CONTROL_INVALID_COMMAND;
    if (xSemaphoreTake(control->mutex, 0U) != pdTRUE) return DAQC_CONTROL_BUSY;
    if (command != control->state) {
        (void)xSemaphoreGive(control->mutex);
        return DAQC_CONTROL_STATE_MISMATCH;
    }
    if (profile_id != DAQC_PROFILE_ESP32) {
        control->profile_applied = false;
        control->configuration_applied = false;
        (void)xSemaphoreGive(control->mutex);
        return DAQC_CONTROL_PROFILE;
    }
    control->profile_applied = true;
    if (apply_configuration) {
        if (command == DAQC_COMMAND_STREAMING || !configuration) {
            control->configuration_applied = false;
            (void)xSemaphoreGive(control->mutex);
            return DAQC_CONTROL_STREAMING;
        }
        control->configuration_applied = DaqcProfileConfigure(configuration);
        if (!control->configuration_applied) {
            (void)xSemaphoreGive(control->mutex);
            return DAQC_CONTROL_CONFIGURATION;
        }
    }
    (void)xSemaphoreGive(control->mutex);
    return DAQC_CONTROL_OK;
}
