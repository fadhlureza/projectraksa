#pragma once

#include "esp_adc/adc_oneshot.h"
#include <stdint.h>

void mq2_init(adc_channel_t channel, adc_oneshot_unit_handle_t *out_handle);
int mq2_read_raw(adc_oneshot_unit_handle_t handle, adc_channel_t channel);