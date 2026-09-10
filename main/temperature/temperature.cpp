#include "temperature.h"

void ds18b20_init_sensor(gpio_num_t pin, onewire_bus_handle_t *out_bus_handle,
                         ds18b20_device_handle_t *out_ds_handle) {
  onewire_bus_config_t bus_config = {};
  bus_config.bus_gpio_num = pin;

  onewire_bus_rmt_config_t rmt_config = {};
  rmt_config.max_rx_bytes = 10;

  onewire_new_bus_rmt(&bus_config, &rmt_config, out_bus_handle);

  onewire_device_iter_handle_t iter = nullptr;
  onewire_device_t next_onewire_device;

  onewire_new_device_iter(*out_bus_handle, &iter);
  onewire_device_iter_get_next(iter, &next_onewire_device);
  onewire_del_device_iter(iter);

  ds18b20_config_t ds_cfg = {};
  ds18b20_new_device(&next_onewire_device, &ds_cfg, out_ds_handle);

  ds18b20_set_resolution(*out_ds_handle, DS18B20_RESOLUTION_12B);
}

float ds18b20_read_temp(ds18b20_device_handle_t ds_handle) {
  float temperature = -999.0;
  ds18b20_trigger_temperature_conversion(ds_handle);
  ds18b20_get_temperature(ds_handle, &temperature);
  return temperature;
}