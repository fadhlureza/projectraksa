#include "tool_raksa.h"
#include "raksa_bridge/raksa_bridge.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "cJSON.h"

static const char *TAG = "tool_raksa";

/* Trip threshold limits for software interlock */
#define SAFETY_MAX_TEMP_C      65.0f
#define SAFETY_MAX_CURRENT_A   16.0f
#define SAFETY_SMOKE_VOLTS     1.6f

esp_err_t tool_raksa_status_execute(const char *input_json, char *output, size_t output_size)
{
    (void)input_json;
    raksa_telemetry_t t;
    esp_err_t err = raksa_get_telemetry(&t);

    if (err != ESP_OK) {
        snprintf(output, output_size, "Error: Gagal membaca data telemetri dari SensorManager (%s)", esp_err_to_name(err));
        return err;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        snprintf(output, output_size, "Error: Memori tidak mencukupi untuk membuat JSON status");
        return ESP_ERR_NO_MEM;
    }

    /* Electrical parameters */
    cJSON_AddNumberToObject(root, "voltage_v", t.voltage);
    cJSON_AddNumberToObject(root, "current_a", t.current);
    cJSON_AddNumberToObject(root, "power_w", t.power);
    cJSON_AddNumberToObject(root, "energy_kwh", t.energy / 1000.0f);
    cJSON_AddNumberToObject(root, "frequency_hz", t.frequency);
    cJSON_AddNumberToObject(root, "power_factor", t.power_factor);

    /* Environmental & physical parameters */
    cJSON_AddNumberToObject(root, "temperature_c", t.temperature);
    cJSON_AddNumberToObject(root, "smoke_voltage_v", t.smoke_voltage);
    cJSON_AddBoolToObject(root, "smoke_alert", t.smoke_detected);
    cJSON_AddBoolToObject(root, "door_open", t.door_open);

    /* Backup Battery */
    cJSON_AddNumberToObject(root, "battery_voltage_v", t.battery_voltage);
    cJSON_AddNumberToObject(root, "battery_pct", t.battery_pct);

    /* Actuator & System Status */
    cJSON_AddNumberToObject(root, "relay_state", t.relay_state ? 1 : 0);
    cJSON_AddNumberToObject(root, "free_heap_bytes", (double)esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "uptime_sec", (double)(esp_timer_get_time() / 1000000ULL));

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_str) {
        snprintf(output, output_size, "Error: Gagal memformat string JSON");
        return ESP_ERR_NO_MEM;
    }

    strncpy(output, json_str, output_size - 1);
    output[output_size - 1] = '\0';
    free(json_str);

    ESP_LOGI(TAG, "raksa_status: V=%.1fV I=%.2fA P=%.1fW T=%.1fC Smoke=%.2fV Relay=%d",
             t.voltage, t.current, t.power, t.temperature, t.smoke_voltage, t.relay_state);
    return ESP_OK;
}
