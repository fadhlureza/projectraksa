#pragma once

#include "driver/gpio.h"
#include "onewire_bus.h"
#include "ds18b20.h"

void ds18b20_init_sensor(gpio_num_t pin, onewire_bus_handle_t *out_bus_handle,
                         ds18b20_device_handle_t *out_ds_handle);

void ds18b20_trigger(ds18b20_device_handle_t ds_handle);
float ds18b20_get(ds18b20_device_handle_t ds_handle);