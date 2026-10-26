#pragma once

#include "esp_err.h"
#include "constant.h" // For SystemData_t
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inisialisasi bus SPI dan mount MicroSD card (FATFS)
 * 
 * Jika kartu MicroSD tidak terpasang atau gagal di-mount, fungsi ini
 * tidak akan menyebabkan crash (graceful degradation), melainkan hanya
 * mencatat log warning dan menonaktifkan fitur logging SD sementara.
 * 
 * @return esp_err_t ESP_OK jika MicroSD terpasang dan siap digunakan
 */
esp_err_t sdlog_init(void);

/**
 * @brief Mengecek apakah MicroSD card berhasil di-mount dan dapat ditulis
 * 
 * @return true jika MicroSD siap
 */
bool sdlog_is_available(void);

/**
 * @brief Menulis satu baris data telemetri RAKSA ke file CSV harian
 * 
 * File otomatis dirotasi per hari: /sdcard/YYYY-MM-DD.csv.
 * Jika file baru dibuat, header CSV akan otomatis ditambahkan di baris pertama.
 * 
 * @param data Pointer ke struct data telemetri sistem
 * @param timeinfo Pointer ke struct tm waktu saat data diambil
 * @return esp_err_t ESP_OK jika penulisan berhasil
 */
esp_err_t sdlog_write_row(const SystemData_t *data, const struct tm *timeinfo);

/**
 * @brief Unmount kartu MicroSD secara aman
 */
void sdlog_unmount(void);

#ifdef __cplusplus
}
#endif
