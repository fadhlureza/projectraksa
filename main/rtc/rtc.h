#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inisialisasi bus I2C dan verifikasi modul RTC DS3231
 * 
 * @param sda_pin GPIO pin SDA (default: PIN_I2C_SDA / GPIO 8)
 * @param scl_pin GPIO pin SCL (default: PIN_I2C_SCL / GPIO 9)
 * @return esp_err_t ESP_OK jika berhasil mendeteksi DS3231 di alamat 0x68
 */
esp_err_t ds3231_rtc_init(gpio_num_t sda_pin, gpio_num_t scl_pin);

/**
 * @brief Membaca waktu aktual dari RTC DS3231 ke struct tm
 * 
 * @param timeinfo Pointer ke struct tm untuk menyimpan hasil pembacaan
 * @return esp_err_t ESP_OK jika berhasil membaca
 */
esp_err_t rtc_get_time(struct tm *timeinfo);

/**
 * @brief Mengatur waktu baru pada RTC DS3231
 * 
 * @param timeinfo Pointer ke struct tm yang berisi waktu baru
 * @return esp_err_t ESP_OK jika berhasil menulis ke RTC
 */
esp_err_t rtc_set_time(const struct tm *timeinfo);

/**
 * @brief Membaca waktu dari DS3231 dan menyinkronkan jam sistem ESP32 (settimeofday)
 * 
 * Setelah dipanggil, fungsi standar C time() dan gettimeofday() akan mengembalikan
 * waktu aktual tanpa perlu koneksi internet NTP.
 * 
 * @return esp_err_t ESP_OK jika jam sistem berhasil disinkronkan
 */
esp_err_t rtc_sync_system_time(void);

/**
 * @brief Membaca sensor suhu internal DS3231
 * 
 * @param[out] temp Pointer untuk menyimpan nilai suhu (derajat Celcius)
 * @return esp_err_t ESP_OK jika berhasil
 */
esp_err_t rtc_get_temperature(float *temp);

/**
 * @brief Mengecek apakah modul RTC terdeteksi dan aktif
 * 
 * @return true jika RTC DS3231 siap digunakan
 */
bool rtc_is_available(void);

#ifdef __cplusplus
}
#endif
