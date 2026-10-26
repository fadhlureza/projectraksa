#include "rtc.h"
#include "constant.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "RTC_DS3231";
static const i2c_port_t I2C_PORT = I2C_NUM_0;

#define DS3231_ADDR         0x68
#define DS3231_REG_TIME     0x00
#define DS3231_REG_CONTROL  0x0E
#define DS3231_REG_STATUS   0x0F
#define DS3231_REG_TEMP_MSB 0x11

static bool s_rtc_initialized = false;

static inline uint8_t bcd_to_dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static inline uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

esp_err_t ds3231_rtc_init(gpio_num_t sda_pin, gpio_num_t scl_pin) {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = scl_pin;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000; // 100 kHz standard mode

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C params: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) { // INVALID_STATE = already installed
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(err));
        return err;
    }

    // Ping DS3231 at 0x68 to verify connection
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "DS3231 not responding on I2C address 0x%02X (%s)", DS3231_ADDR, esp_err_to_name(err));
        s_rtc_initialized = false;
        return err;
    }

    // Verify and clear Oscillator Stop Flag (OSF) in status register (0x0F)
    uint8_t status = 0;
    uint8_t reg_status = DS3231_REG_STATUS;
    if (i2c_master_write_read_device(I2C_PORT, DS3231_ADDR, &reg_status, 1, &status, 1, pdMS_TO_TICKS(100)) == ESP_OK) {
        if (status & 0x80) {
            ESP_LOGW(TAG, "Oscillator stopped flag detected. Oscillator was reset/battery changed.");
            uint8_t clear_osf[2] = {DS3231_REG_STATUS, (uint8_t)(status & ~0x80)};
            i2c_master_write_to_device(I2C_PORT, DS3231_ADDR, clear_osf, sizeof(clear_osf), pdMS_TO_TICKS(100));
        }
    }

    s_rtc_initialized = true;
    ESP_LOGI(TAG, "DS3231 RTC successfully initialized on SDA=%d, SCL=%d", sda_pin, scl_pin);

    // Sync system time immediately on boot
    rtc_sync_system_time();
    return ESP_OK;
}

esp_err_t rtc_get_time(struct tm *timeinfo) {
    if (!s_rtc_initialized || timeinfo == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t reg = DS3231_REG_TIME;
    uint8_t data[7] = {0};

    esp_err_t err = i2c_master_write_read_device(I2C_PORT, DS3231_ADDR, &reg, 1, data, sizeof(data), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time registers: %s", esp_err_to_name(err));
        return err;
    }

    memset(timeinfo, 0, sizeof(struct tm));
    timeinfo->tm_sec  = bcd_to_dec(data[0] & 0x7F);
    timeinfo->tm_min  = bcd_to_dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd_to_dec(data[2] & 0x3F); // 24-hour mode
    timeinfo->tm_wday = (data[3] & 0x07) - 1;       // 1-7 in DS3231, 0-6 in struct tm
    timeinfo->tm_mday = bcd_to_dec(data[4] & 0x3F);
    timeinfo->tm_mon  = bcd_to_dec(data[5] & 0x1F) - 1; // 1-12 in DS3231, 0-11 in struct tm
    timeinfo->tm_year = bcd_to_dec(data[6]) + 100;      // 00-99 -> years since 1900 (100 = 2000)

    return ESP_OK;
}

esp_err_t rtc_set_time(const struct tm *timeinfo) {
    if (!s_rtc_initialized || timeinfo == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[8];
    data[0] = DS3231_REG_TIME;
    data[1] = dec_to_bcd(timeinfo->tm_sec);
    data[2] = dec_to_bcd(timeinfo->tm_min);
    data[3] = dec_to_bcd(timeinfo->tm_hour);
    data[4] = (uint8_t)(timeinfo->tm_wday + 1);
    data[5] = dec_to_bcd(timeinfo->tm_mday);
    data[6] = dec_to_bcd(timeinfo->tm_mon + 1);
    data[7] = dec_to_bcd(timeinfo->tm_year >= 100 ? (timeinfo->tm_year - 100) : timeinfo->tm_year);

    esp_err_t err = i2c_master_write_to_device(I2C_PORT, DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "RTC time updated to: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        // Also sync local ESP32 system clock
        rtc_sync_system_time();
    } else {
        ESP_LOGE(TAG, "Failed to write time registers: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t rtc_sync_system_time(void) {
    struct tm tm_val;
    esp_err_t err = rtc_get_time(&tm_val);
    if (err != ESP_OK) {
        return err;
    }

    time_t t = mktime(&tm_val);
    if (t == (time_t)-1) {
        return ESP_FAIL;
    }

    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, nullptr);

    ESP_LOGI(TAG, "System time synchronized with DS3231: %04d-%02d-%02d %02d:%02d:%02d",
             tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
             tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
    return ESP_OK;
}

esp_err_t rtc_get_temperature(float *temp) {
    if (!s_rtc_initialized || temp == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t reg = DS3231_REG_TEMP_MSB;
    uint8_t data[2] = {0};

    esp_err_t err = i2c_master_write_read_device(I2C_PORT, DS3231_ADDR, &reg, 1, data, sizeof(data), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        return err;
    }

    int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    *temp = (float)(raw >> 6) * 0.25f;
    return ESP_OK;
}

bool rtc_is_available(void) {
    return s_rtc_initialized;
}
