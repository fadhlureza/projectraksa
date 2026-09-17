#pragma once

// mc38 door sensor
#define PIN_DOOR_SENSOR GPIO_NUM_15

// ds18b20 temperature sensor
#define PIN_TEMP_SENSOR GPIO_NUM_7

// mq2 smoke sensor
#define PIN_SMOKE_SENSOR GPIO_NUM_6

// pzem-004t
#define PIN_PZEM_RX GPIO_NUM_44
#define PIN_PZEM_TX GPIO_NUM_43

// i2c config
#define PIN_I2C_SDA GPIO_NUM_8
#define PIN_I2C_SCL GPIO_NUM_9

// rtc


// sensor config
#define SENSOR_POLL_RATE_MS 500

// Structure to store shared system telemetry data
typedef struct {
  bool door_open;
  float smoke_voltage;
  float temperature;

  // Power Quality Data
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float power_factor;

  // Battery Data
  float battery_voltage;
  int battery_percentage;
} SystemData_t;