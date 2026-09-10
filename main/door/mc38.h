#pragma once

#include "driver/gpio.h"
#include <stdbool.h>

void mc38_init(gpio_num_t pin);
bool mc38_is_door_open(gpio_num_t pin);