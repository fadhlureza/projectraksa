#pragma once

// mc38 door sensor
#define PIN_DOOR_SENSOR GPIO_NUM_15

// ds18b20 temperature sensor
#define PIN_TEMP_SENSOR GPIO_NUM_7

// mq2 smoke sensor
#define PIN_SMOKE_SENSOR GPIO_NUM_6

// relay power outlet
#define PIN_RELAY GPIO_NUM_14

// pzem-004t
#define PIN_PZEM_RX GPIO_NUM_18
#define PIN_PZEM_TX GPIO_NUM_17

// i2c config
#define PIN_I2C_SDA GPIO_NUM_8
#define PIN_I2C_SCL GPIO_NUM_9

// rtc ds3231
#define RTC_I2C_ADDR 0x68

// microsd card spi config (placeholder pins; avoiding GPIO 14 used by PIN_RELAY)
#define PIN_SD_CS   GPIO_NUM_11
#define PIN_SD_SCK  GPIO_NUM_12
#define PIN_SD_MOSI GPIO_NUM_13
#define PIN_SD_MISO GPIO_NUM_10

// sensor config
#define SENSOR_POLL_RATE_MS 500
#define SD_LOG_INTERVAL_MS  5000

// Deterministic safety protection thresholds (Hardware Trips)
#define THRESHOLD_SMOKE_VOLTAGE  1.6f  // V: smoke/gas danger threshold
#define THRESHOLD_TEMP_CRITICAL  65.0f // C: fire hazard / overheat
#define THRESHOLD_CURRENT_MAX    16.0f // A: overcurrent limit

// Structure to store shared system telemetry data
typedef struct {
  bool door_open;
  float smoke_voltage;
  float temperature;
  bool relay_state;

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