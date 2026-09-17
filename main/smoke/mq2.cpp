#include "mq2.h"
#include "esp_log.h"

void mq2_init(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_DEFAULT;
    config.atten = ADC_ATTEN_DB_12;

    adc_oneshot_config_channel(handle, channel, &config);
}

// ADC max value for 12-bit width
#define ADC_MAX_VAL 4095.0f
// Reference voltage for 12dB attenuation (approx 3.3V)
#define ADC_REF_VOLTAGE 3.3f
// Voltage divider multiplier for MQ2 (4.7k : 4.7k) => V_out = V_adc * 2
#define MQ2_DIVIDER_MULTIPLIER 2.0f

float mq2_read_voltage(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    int raw_val = 0;
    adc_oneshot_read(handle, channel, &raw_val);

    // Calculate voltage at the ADC pin
    float pin_voltage = (raw_val / ADC_MAX_VAL) * ADC_REF_VOLTAGE;
    
    // Calculate actual sensor voltage based on divider
    float sensor_voltage = pin_voltage * MQ2_DIVIDER_MULTIPLIER;
    
    return sensor_voltage;
}