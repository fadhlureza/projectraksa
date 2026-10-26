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

esp_err_t tool_raksa_relay_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root) {
        snprintf(output, output_size, "Error: Format JSON input tidak valid");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *state_item = cJSON_GetObjectItem(root, "state");
    cJSON *confirm_item = cJSON_GetObjectItem(root, "confirm");

    if (!state_item || !cJSON_IsNumber(state_item)) {
        cJSON_Delete(root);
        snprintf(output, output_size, "Error: Parameter 'state' (0 atau 1) wajib disertakan");
        return ESP_ERR_INVALID_ARG;
    }

    int requested_state = state_item->valueint;
    bool is_confirmed = (confirm_item && cJSON_IsTrue(confirm_item));
    cJSON_Delete(root);

    if (requested_state != 0 && requested_state != 1) {
        snprintf(output, output_size, "Error: Parameter 'state' harus bernilai 0 (OFF) atau 1 (ON)");
        return ESP_ERR_INVALID_ARG;
    }

    /* Check 1: Two-step explicit confirmation */
    if (!is_confirmed) {
        if (requested_state == 0) {
            snprintf(output, output_size,
                     "PERINGATAN KONFIRMASI: Mematikan relai (state=0) akan MEMUTUS seluruh aliran listrik beban gardu/panel. "
                     "Minta konfirmasi eksplisit dari operator ('Apakah Anda yakin ingin mematikan relai beban sekarang?'). "
                     "Jika operator telah menyetujui, panggil kembali tool ini dengan parameter \"confirm\": true.");
        } else {
            snprintf(output, output_size,
                     "PERINGATAN KONFIRMASI: Menyalakan relai (state=1) akan MENGHUBUNGKAN aliran listrik beban. "
                     "Pastikan operator telah memeriksa kondisi fisik gardu dan panggil kembali dengan \"confirm\": true.");
        }
        ESP_LOGW(TAG, "raksa_relay: Action requires confirmation (state=%d)", requested_state);
        return ESP_OK;
    }

    /* Check 2: Safety interlock when turning ON */
    if (requested_state == 1) {
        raksa_telemetry_t t;
        esp_err_t err = raksa_get_telemetry(&t);
        if (err == ESP_OK) {
            if (t.smoke_voltage > SAFETY_SMOKE_VOLTS) {
                snprintf(output, output_size,
                         "DITOLAK: Sensor asap MQ-2 mendeteksi gas/asap berbahaya (%.2fV > %.2fV). "
                         "Relai DILARANG dinyalakan demi keselamatan kebakaran!",
                         t.smoke_voltage, SAFETY_SMOKE_VOLTS);
                ESP_LOGE(TAG, "raksa_relay: Interlock prevented relay ON due to smoke");
                return ESP_ERR_INVALID_STATE;
            }
            if (t.temperature > SAFETY_MAX_TEMP_C) {
                snprintf(output, output_size,
                         "DITOLAK: Suhu gardu terlampau tinggi (%.1f°C > %.1f°C). "
                         "Relai DILARANG dinyalakan sampai suhu normal kembali!",
                         t.temperature, SAFETY_MAX_TEMP_C);
                ESP_LOGE(TAG, "raksa_relay: Interlock prevented relay ON due to temperature");
                return ESP_ERR_INVALID_STATE;
            }
            if (t.current > SAFETY_MAX_CURRENT_A) {
                snprintf(output, output_size,
                         "DITOLAK: Arus listrik terukur melampaui batas aman (%.2fA > %.1fA). "
                         "Relai DILARANG dinyalakan untuk mencegah lonjakan overcurrent!",
                         t.current, SAFETY_MAX_CURRENT_A);
                ESP_LOGE(TAG, "raksa_relay: Interlock prevented relay ON due to current");
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    /* Execute relay switch through bridge */
    esp_err_t set_err = raksa_set_relay(requested_state == 1);
    if (set_err != ESP_OK) {
        snprintf(output, output_size, "Error: Gagal mengubah status relai (%s)", esp_err_to_name(set_err));
        return set_err;
    }

    if (requested_state == 1) {
        snprintf(output, output_size, "OK: Relai beban berhasil DIAKTIFKAN (ON). Aliran listrik ke beban telah terhubung kembali.");
    } else {
        snprintf(output, output_size, "OK: Relai beban berhasil DINONAKTIFKAN (OFF). Aliran listrik ke beban telah diputus dengan aman.");
    }

    ESP_LOGI(TAG, "raksa_relay: Successfully set relay state to %d", requested_state);
    return ESP_OK;
}
