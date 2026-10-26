#include "raksa_bridge.h"
#include "sensor_manager/sensor.h"
#include "rtc/rtc.h"
#include "constant.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "RAKSA_BRIDGE";

extern "C" {

esp_err_t raksa_get_telemetry(raksa_telemetry_t *telemetry) {
    if (telemetry == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    SystemData_t data = SensorManager::get_data();
    telemetry->voltage          = data.voltage;
    telemetry->current          = data.current;
    telemetry->power            = data.power;
    telemetry->energy           = data.energy;
    telemetry->frequency        = data.frequency;
    telemetry->power_factor     = data.power_factor;
    telemetry->temperature      = data.temperature;
    telemetry->door_open        = data.door_open;
    telemetry->smoke_voltage    = data.smoke_voltage;
    telemetry->smoke_detected   = (data.smoke_voltage > THRESHOLD_SMOKE_VOLTAGE);
    telemetry->battery_voltage  = data.battery_voltage;
    telemetry->battery_pct      = data.battery_percentage;
    telemetry->relay_state      = data.relay_state;

    return ESP_OK;
}

esp_err_t raksa_set_relay(bool state) {
    SensorManager::set_relay(state);
    ESP_LOGI(TAG, "Relay commanded to %s via raksa_bridge", state ? "ON" : "OFF");
    return ESP_OK;
}

bool raksa_get_relay(void) {
    SystemData_t data = SensorManager::get_data();
    return data.relay_state;
}

esp_err_t raksa_rtc_now(struct tm *timeinfo) {
    return rtc_get_time(timeinfo);
}

int raksa_format_telemetry_json(char *buffer, size_t buffer_size) {
    if (buffer == nullptr || buffer_size == 0) {
        return 0;
    }

    raksa_telemetry_t t;
    raksa_get_telemetry(&t);

    return snprintf(buffer, buffer_size,
        "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy\":%.0f,"
        "\"frequency\":%.1f,\"power_factor\":%.2f,\"temperature\":%.2f,"
        "\"door_open\":%s,\"smoke_detected\":%s,\"smoke_voltage\":%.2f,"
        "\"battery_voltage\":%.2f,\"battery_pct\":%d,\"relay_state\":%s}",
        t.voltage, t.current, t.power, t.energy,
        t.frequency, t.power_factor, t.temperature,
        t.door_open ? "true" : "false",
        t.smoke_detected ? "true" : "false", t.smoke_voltage,
        t.battery_voltage, t.battery_pct,
        t.relay_state ? "true" : "false");
}

} // extern "C"
