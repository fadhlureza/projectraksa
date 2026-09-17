#include "battery.h"
#include "esp_log.h"

// ADC max value for 12-bit width
#define ADC_MAX_VAL 4095.0f
// Reference voltage for 12dB attenuation (approx 3.3V)
#define ADC_REF_VOLTAGE 3.3f
// Voltage divider multiplier (100k : 100k) => V_bat = V_adc * 2
#define DIVIDER_MULTIPLIER 2.0f

// 18650 Battery characteristics
#define BATTERY_MAX_VOLTAGE 4.2f
#define BATTERY_MIN_VOLTAGE 3.2f

void battery_init(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_DEFAULT;
    config.atten = ADC_ATTEN_DB_12;

    adc_oneshot_config_channel(handle, channel, &config);
}

float battery_read_voltage(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    int raw_val = 0;
    adc_oneshot_read(handle, channel, &raw_val);

    // Calculate voltage at the ADC pin
    float pin_voltage = (raw_val / ADC_MAX_VAL) * ADC_REF_VOLTAGE;
    
    // Calculate actual battery voltage based on divider
    float battery_voltage = pin_voltage * DIVIDER_MULTIPLIER;
    
    return battery_voltage;
}

int battery_get_percentage(float voltage) {
    if (voltage >= BATTERY_MAX_VOLTAGE) {
        return 100;
    } else if (voltage <= BATTERY_MIN_VOLTAGE) {
        return 0;
    }
    
    // Linear mapping for the 18650 curve (approximation)
    float percentage = ((voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f;
    
    return (int)percentage;
}
