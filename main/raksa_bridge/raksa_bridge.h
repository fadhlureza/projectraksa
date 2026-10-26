#pragma once

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Plain-C telemetry data structure for MimiClaw C-agent
 */
typedef struct {
    float voltage;          // Tegangan AC (V)
    float current;          // Arus beban (A)
    float power;            // Daya aktif (W)
    float energy;           // Energi kumulatif (Wh)
    float frequency;        // Frekuensi jala-jala (Hz)
    float power_factor;     // Faktor daya (0.00 - 1.00)
    float temperature;      // Suhu boks/panel (derajat Celcius)
    bool  door_open;        // Status pintu (true: terbuka, false: tertutup)
    bool  smoke_detected;   // Indikasi asap/gas terdeteksi
    float smoke_voltage;    // Tegangan analog MQ-2 (V)
    float battery_voltage;  // Tegangan baterai backup (V)
    int   battery_pct;      // Persentase baterai backup (%)
    bool  relay_state;      // Status relai (true: ON/aliran daya menyala, false: OFF/terputus)
} raksa_telemetry_t;

/**
 * @brief Thread-safe getter to fetch a snapshot of all system telemetry
 * @param[out] telemetry Pointer to raksa_telemetry_t struct to populate
 * @return esp_err_t ESP_OK on success
 */
esp_err_t raksa_get_telemetry(raksa_telemetry_t *telemetry);

/**
 * @brief Thread-safe control of the power outlet relay
 * @param state true to turn ON, false to turn OFF
 * @return esp_err_t ESP_OK on success
 */
esp_err_t raksa_set_relay(bool state);

/**
 * @brief Thread-safe query of current relay state
 * @return true if relay is currently ON, false if OFF
 */
bool raksa_get_relay(void);

/**
 * @brief Query current RTC time for MimiClaw agent
 * @param[out] timeinfo Pointer to struct tm to populate
 * @return esp_err_t ESP_OK on success
 */
esp_err_t raksa_rtc_now(struct tm *timeinfo);

/**
 * @brief Format current telemetry into a JSON string for LLM tool outputs
 * @param[out] buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return int Number of bytes written
 */
int raksa_format_telemetry_json(char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
