#pragma once

#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ADC channel for the battery voltage divider.
 * 
 * @param channel The ADC channel connected to the divider midpoint.
 * @param out_handle Pointer to store the ADC oneshot unit handle.
 */
void battery_init(adc_oneshot_unit_handle_t handle, adc_channel_t channel);

/**
 * @brief Read the actual battery voltage (in Volts).
 * 
 * @param handle The ADC oneshot unit handle.
 * @param channel The ADC channel to read from.
 * @return float The calculated battery voltage.
 */
float battery_read_voltage(adc_oneshot_unit_handle_t handle, adc_channel_t channel);

/**
 * @brief Convert battery voltage to a 0-100% scale for an 18650 Li-ion cell.
 * 
 * @param voltage The measured battery voltage.
 * @return int Percentage (0 to 100).
 */
int battery_get_percentage(float voltage);

#ifdef __cplusplus
}
#endif
