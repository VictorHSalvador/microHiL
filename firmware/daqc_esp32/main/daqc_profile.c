#include "daqc_profile.h"

#include <math.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/dac_oneshot.h"

static const gpio_num_t g_di_pins[] = {GPIO_NUM_4, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_27};
static const gpio_num_t g_do_pins[] = {GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23};
static const gpio_num_t g_pwm_pins[] = {GPIO_NUM_18, GPIO_NUM_19};
static dac_oneshot_handle_t g_dac_channels[2];
static bool g_configured;

static float ReadFloat(const uint8_t *bytes) { float value; memcpy(&value, bytes, sizeof(value)); return value; }
static void WriteFloat(uint8_t *bytes, float value) { memcpy(bytes, &value, sizeof(value)); }

bool DaqcProfileConfigure(const daqc_configuration_t *configuration) {
    if (!configuration || configuration->adc_resolution_bits < 9U || configuration->adc_resolution_bits > 12U) return false;
    for (size_t index = 0U; index < 6U; ++index) if (configuration->adc_attenuation[index] > 3U) return false;
    for (size_t index = 0U; index < 2U; ++index) if (configuration->pwm_frequency_hz[index] == 0U || configuration->pwm_resolution_bits[index] == 0U || configuration->pwm_resolution_bits[index] > 20U) return false;
    for (size_t index = 0U; index < 4U; ++index) if (gpio_set_direction(g_di_pins[index], GPIO_MODE_INPUT) != ESP_OK) return false;
    for (size_t index = 0U; index < 5U; ++index) if (gpio_set_direction(g_do_pins[index], GPIO_MODE_OUTPUT) != ESP_OK) return false;
    for (size_t index = 0U; index < 2U; ++index) {
        const ledc_timer_config_t timer = {.speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = configuration->pwm_resolution_bits[index], .timer_num = (ledc_timer_t)index,
                                            .freq_hz = configuration->pwm_frequency_hz[index], .clk_cfg = LEDC_AUTO_CLK};
        const ledc_channel_config_t channel = {.gpio_num = g_pwm_pins[index], .speed_mode = LEDC_LOW_SPEED_MODE, .channel = (ledc_channel_t)index,
                                                .intr_type = LEDC_INTR_DISABLE, .timer_sel = (ledc_timer_t)index, .duty = 0U, .hpoint = 0U};
        if (ledc_timer_config(&timer) != ESP_OK || ledc_channel_config(&channel) != ESP_OK) return false;
    }
    g_configured = true;
    DaqcProfileSetSafeOutputs();
    return true;
}

bool DaqcProfileApplyActuation(const uint8_t payload[21]) {
    if (!g_configured || !payload) return false;
    for (size_t index = 0U; index < 2U; ++index) { const float duty = ReadFloat(payload + 2U + index * 4U); if (!isfinite(duty) || duty < 0.0F || duty > 1.0F) return false; }
    for (size_t index = 0U; index < 5U; ++index) if (payload[index < 2U ? index : index + 8U] > 1U) return false;
    return true;
}
bool DaqcProfileAcquire(uint8_t payload[28]) { if (!g_configured || !payload) return false; for (size_t index = 0U; index < 4U; ++index) payload[index] = (uint8_t)gpio_get_level(g_di_pins[index]); for (size_t index = 0U; index < 6U; ++index) WriteFloat(payload + 4U + index * 4U, NAN); return true; }
void DaqcProfileSetSafeOutputs(void) { for (size_t index = 0U; index < 5U; ++index) (void)gpio_set_level(g_do_pins[index], 0); for (size_t index = 0U; index < 2U; ++index) { (void)ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)index, 0U); (void)ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)index); } }
