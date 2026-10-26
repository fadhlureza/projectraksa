#include "tool_get_time.h"
#include "raksa_bridge/raksa_bridge.h"
#include <stdio.h>
#include <time.h>
#include "esp_log.h"

static const char *TAG = "tool_time";

esp_err_t tool_get_time_init(void)
{
    ESP_LOGI(TAG, "Hardware RTC time tool initialized");
    return ESP_OK;
}

esp_err_t tool_get_time_execute(const char *input_json, char *output, size_t output_size)
{
    (void)input_json;
    struct tm tm_info;
    esp_err_t err = raksa_rtc_now(&tm_info);
    if (err != ESP_OK) {
        snprintf(output, output_size, "Error: Gagal membaca waktu dari RTC (%s)", esp_err_to_name(err));
        return err;
    }

    time_t now = mktime(&tm_info);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S WIB", &tm_info);

    snprintf(output, output_size, "Waktu saat ini: %s (Unix Epoch: %lld)", time_str, (long long)now);
    ESP_LOGI(TAG, "tool_get_time: %s", output);
    return ESP_OK;
}
