#pragma once

#include "esp_adc/adc_oneshot.h"
#include <stdint.h>

void mq2_init(adc_oneshot_unit_handle_t handle, adc_channel_t channel);
float mq2_read_voltage(adc_oneshot_unit_handle_t handle, adc_channel_t channel);