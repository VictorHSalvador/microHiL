uint8 DISABLE=1
uint8 ENABLE=2
uint8 STREAMING=3
uint8 ADC_ATTENUATION_0_DB=0
uint8 ADC_ATTENUATION_2_5_DB=1
uint8 ADC_ATTENUATION_6_DB=2
uint8 ADC_ATTENUATION_11_DB=3

uint8 command
uint32 profile_id
uint8 apply_configuration
uint8 adc_resolution_bits
uint8[6] adc_attenuation
uint32[2] pwm_frequency_hz
uint8[2] pwm_resolution_bits
