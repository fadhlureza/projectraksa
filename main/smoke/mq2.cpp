#include "mq2.h"
#include "esp_log.h"

void mq2_init(adc_channel_t channel, adc_oneshot_unit_handle_t *out_handle) {
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;

    adc_oneshot_new_unit(&init_config, out_handle);

    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_DEFAULT;
    config.atten = ADC_ATTEN_DB_12;

    adc_oneshot_config_channel(*out_handle, channel, &config);
}

int mq2_read_raw(adc_oneshot_unit_handle_t handle, adc_channel_t channel) {
    int raw_val = 0;
    adc_oneshot_read(handle, channel, &raw_val);
    return raw_val;
}