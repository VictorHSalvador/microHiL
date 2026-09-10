#include "daqc_profile.h"
#include "daqc_protocol.h"

#include <math.h>
#include <string.h>

#include "driver/dac_oneshot.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#define DAQC_ANALOG_REFERENCE_VOLTS 3.3F
#define DAQC_ADC_DEFAULT_VREF_MV 1100U

static const gpio_num_t g_di_pins[] = {GPIO_NUM_4, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_27};
static const gpio_num_t g_do_pins[] = {GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23};
static const gpio_num_t g_pwm_pins[] = {GPIO_NUM_18, GPIO_NUM_19};
static const gpio_num_t g_ai_pins[] = {GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_39};
static const dac_channel_t g_dac_ids[] = {DAC_CHAN_0, DAC_CHAN_1};

static adc_oneshot_unit_handle_t g_adc_unit;
static adc_channel_t g_adc_channels[6];
static adc_cali_handle_t g_adc_calibration[6];
static dac_oneshot_handle_t g_dac_channels[2];
static daqc_configuration_t g_configuration;
static bool g_configured;

static float ReadFloat(const uint8_t *bytes) {
    float value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static void WriteFloat(uint8_t *bytes, float value) {
    memcpy(bytes, &value, sizeof(value));
}

static bool IsBinary(uint8_t value) {
    return value == 0U || value == 1U;
}

static void ReleaseAdc(void) {
    for (size_t index = 0U; index < 6U; ++index) {
        if (g_adc_calibration[index]) {
            (void)adc_cali_delete_scheme_line_fitting(g_adc_calibration[index]);
            g_adc_calibration[index] = NULL;
        }
    }
    if (g_adc_unit) {
        (void)adc_oneshot_del_unit(g_adc_unit);
        g_adc_unit = NULL;
    }
}

static void ReleaseDac(void) {
    for (size_t index = 0U; index < 2U; ++index) {
        if (g_dac_channels[index]) {
            (void)dac_oneshot_del_channel(g_dac_channels[index]);
            g_dac_channels[index] = NULL;
        }
    }
}

static bool ConfigureAdc(const daqc_configuration_t *configuration) {
    const adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = ADC_UNIT_1, .clk_src = ADC_RTC_CLK_SRC_DEFAULT, .ulp_mode = ADC_ULP_MODE_DISABLE};
    if (adc_oneshot_new_unit(&unit_config, &g_adc_unit) != ESP_OK) return false;

    for (size_t index = 0U; index < 6U; ++index) {
        adc_unit_t unit_id;
        const adc_oneshot_chan_cfg_t channel_config = {
            .atten = (adc_atten_t)configuration->adc_attenuation[index],
            .bitwidth = (adc_bitwidth_t)configuration->adc_resolution_bits,
        };
        const adc_cali_line_fitting_config_t calibration_config = {
            .unit_id = ADC_UNIT_1,
            .atten = (adc_atten_t)configuration->adc_attenuation[index],
            .bitwidth = (adc_bitwidth_t)configuration->adc_resolution_bits,
            .default_vref = DAQC_ADC_DEFAULT_VREF_MV,
        };
        if (adc_oneshot_io_to_channel(g_ai_pins[index], &unit_id, &g_adc_channels[index]) != ESP_OK || unit_id != ADC_UNIT_1 ||
            adc_oneshot_config_channel(g_adc_unit, g_adc_channels[index], &channel_config) != ESP_OK ||
            adc_cali_create_scheme_line_fitting(&calibration_config, &g_adc_calibration[index]) != ESP_OK) {
            ReleaseAdc();
            return false;
        }
    }
    return true;
}

static bool ConfigureDac(void) {
    for (size_t index = 0U; index < 2U; ++index) {
        const dac_oneshot_config_t channel_config = {.chan_id = g_dac_ids[index]};
        if (dac_oneshot_new_channel(&channel_config, &g_dac_channels[index]) != ESP_OK) {
            ReleaseDac();
            return false;
        }
    }
    return true;
}

static bool ConfigurePwm(const daqc_configuration_t *configuration) {
    for (size_t index = 0U; index < 2U; ++index) {
        const ledc_timer_config_t timer_config = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = (ledc_timer_bit_t)configuration->pwm_resolution_bits[index],
            .timer_num = (ledc_timer_t)index,
            .freq_hz = configuration->pwm_frequency_hz[index],
            .clk_cfg = LEDC_AUTO_CLK,
        };
        const ledc_channel_config_t channel_config = {
            .gpio_num = g_pwm_pins[index],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = (ledc_channel_t)index,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = (ledc_timer_t)index,
            .duty = 0U,
            .hpoint = 0U,
        };
        if (ledc_timer_config(&timer_config) != ESP_OK || ledc_channel_config(&channel_config) != ESP_OK) return false;
    }
    return true;
}

bool DaqcProfileConfigure(const daqc_configuration_t *configuration) {
    if (!configuration || configuration->adc_resolution_bits < 9U || configuration->adc_resolution_bits > 12U) return false;
    for (size_t index = 0U; index < 6U; ++index) if (configuration->adc_attenuation[index] > 3U) return false;
    for (size_t index = 0U; index < 2U; ++index) {
        if (configuration->pwm_frequency_hz[index] == 0U || configuration->pwm_resolution_bits[index] == 0U || configuration->pwm_resolution_bits[index] > 20U) return false;
    }

    g_configured = false;
    ReleaseAdc();
    ReleaseDac();
    for (size_t index = 0U; index < 4U; ++index) if (gpio_set_direction(g_di_pins[index], GPIO_MODE_INPUT) != ESP_OK) return false;
    for (size_t index = 0U; index < 5U; ++index) if (gpio_set_direction(g_do_pins[index], GPIO_MODE_OUTPUT) != ESP_OK) return false;
    if (!ConfigureAdc(configuration) || !ConfigureDac() || !ConfigurePwm(configuration)) {
        ReleaseAdc();
        ReleaseDac();
        return false;
    }
    g_configuration = *configuration;
    g_configured = true;
    DaqcProfileSetSafeOutputs();
    return true;
}

bool DaqcProfileApplyActuation(const uint8_t payload[DAQC_ACTUATION_SIZE]) {
    if (!g_configured || !payload) return false;
    if (!IsBinary(payload[0]) || !IsBinary(payload[1]) || !IsBinary(payload[10]) || !IsBinary(payload[11]) || !IsBinary(payload[12])) return false;

    const float pwm_values[2] = {ReadFloat(payload + 2U), ReadFloat(payload + 6U)};
    const float analog_values[2] = {ReadFloat(payload + 13U), ReadFloat(payload + 17U)};
    for (size_t index = 0U; index < 2U; ++index) {
        if (!isfinite(pwm_values[index]) || pwm_values[index] < 0.0F || pwm_values[index] > 1.0F || !isfinite(analog_values[index]) ||
            analog_values[index] < 0.0F || analog_values[index] > DAQC_ANALOG_REFERENCE_VOLTS) return false;
    }

    const uint8_t do_values[5] = {payload[0], payload[1], payload[10], payload[11], payload[12]};
    for (size_t index = 0U; index < 5U; ++index) if (gpio_set_level(g_do_pins[index], do_values[index]) != ESP_OK) return false;
    for (size_t index = 0U; index < 2U; ++index) {
        const uint32_t max_duty = (1UL << g_configuration.pwm_resolution_bits[index]) - 1UL;
        const uint32_t duty = (uint32_t)lroundf(pwm_values[index] * (float)max_duty);
        const uint8_t dac_code = (uint8_t)lroundf(analog_values[index] * 255.0F / DAQC_ANALOG_REFERENCE_VOLTS);
        if (ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, (ledc_channel_t)index, duty, 0U) != ESP_OK ||
            dac_oneshot_output_voltage(g_dac_channels[index], dac_code) != ESP_OK) return false;
    }
    return true;
}

bool DaqcProfileAcquire(uint8_t payload[DAQC_ACQUISITION_SIZE]) {
    if (!g_configured || !payload) return false;
    for (size_t index = 0U; index < 4U; ++index) payload[index] = (uint8_t)gpio_get_level(g_di_pins[index]);
    for (size_t index = 0U; index < 6U; ++index) {
        int millivolts;
        if (adc_oneshot_get_calibrated_result(g_adc_unit, g_adc_calibration[index], g_adc_channels[index], &millivolts) != ESP_OK) return false;
        WriteFloat(payload + 4U + index * sizeof(float), (float)millivolts / 1000.0F);
    }
    return true;
}

void DaqcProfileSetSafeOutputs(void) {
    for (size_t index = 0U; index < 5U; ++index) (void)gpio_set_level(g_do_pins[index], 0);
    for (size_t index = 0U; index < 2U; ++index) {
        if (g_configuration.pwm_resolution_bits[index]) (void)ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, (ledc_channel_t)index, 0U, 0U);
        if (g_dac_channels[index]) (void)dac_oneshot_output_voltage(g_dac_channels[index], 0U);
    }
}
