#include "sdlog.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *TAG = "SD_LOGGER";
static const char *MOUNT_POINT = "/sdcard";

static bool s_sd_mounted = false;
static sdmmc_card_t *s_card = nullptr;
static spi_host_device_t s_spi_host = SPI2_HOST;

esp_err_t sdlog_init(void) {
    ESP_LOGI(TAG, ">> Initializing MicroSD Logger via SPI...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 4;
    mount_config.allocation_unit_size = 16 * 1024;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = PIN_SD_MOSI;
    bus_cfg.miso_io_num = PIN_SD_MISO;
    bus_cfg.sclk_io_num = PIN_SD_SCK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;

    esp_err_t ret = spi_bus_initialize(s_spi_host, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // INVALID_STATE = already initialized
        ESP_LOGW(TAG, "SPI bus initialize failed (%s)", esp_err_to_name(ret));
        s_sd_mounted = false;
        return ret;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = s_spi_host;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.host_id = s_spi_host;

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "MicroSD not detected / mount failed (%s). System will operate without SD logging.",
                 esp_err_to_name(ret));
        s_sd_mounted = false;
        return ret;
    }

    s_sd_mounted = true;
    ESP_LOGI(TAG, "MicroSD mounted successfully at %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

static TickType_t s_last_remount_attempt = 0;
#define REMOUNT_RETRY_INTERVAL_MS 3000

bool sdlog_is_available(void) {
    if (!s_sd_mounted) {
        // Auto-remount: periodically attempt to mount if user re-inserted the card
        TickType_t now = xTaskGetTickCount();
        if ((now - s_last_remount_attempt) >= pdMS_TO_TICKS(REMOUNT_RETRY_INTERVAL_MS)) {
            s_last_remount_attempt = now;
            sdlog_init();
        }
    }
    return s_sd_mounted;
}

esp_err_t sdlog_write_row(const SystemData_t *data, const struct tm *timeinfo) {
    if (!sdlog_is_available() || data == nullptr || timeinfo == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/%04d-%02d-%02d.csv",
             MOUNT_POINT, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);

    // Check if file exists; if not, we need to create it and write the CSV header
    bool file_exists = (access(filepath, F_OK) == 0);

    FILE *f = fopen(filepath, "a");
    if (f == nullptr) {
        ESP_LOGW(TAG, "Failed to open CSV file %s. MicroSD may have been removed.", filepath);
        // Clean up stale mount so it can auto-remount when inserted back
        sdlog_unmount();
        return ESP_FAIL;
    }

    if (!file_exists) {
        // Write header for first row of the day
        fputs("Timestamp,Voltage_V,Current_A,Power_W,Energy_Wh,Frequency_Hz,PowerFactor,"
              "Temperature_C,DoorOpen,SmokeVoltage_V,SmokeDetected,BattVolt_V,BattPct,RelayState\n", f);
    }

    // Format telemetry row
    fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d,%.1f,%.3f,%.1f,%.0f,%.1f,%.2f,%.2f,%d,%.2f,%d,%.2f,%d,%d\n",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
            data->voltage, data->current, data->power, data->energy,
            data->frequency, data->power_factor, data->temperature,
            data->door_open ? 1 : 0,
            data->smoke_voltage,
            (data->smoke_voltage > 1.6f) ? 1 : 0,
            data->battery_voltage, data->battery_percentage,
            data->relay_state ? 1 : 0);

    fflush(f);
    fclose(f);
    return ESP_OK;
}

void sdlog_unmount(void) {
    if (s_sd_mounted) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
        s_card = nullptr;
        s_sd_mounted = false;
        ESP_LOGI(TAG, "MicroSD unmounted successfully");
    }
}
