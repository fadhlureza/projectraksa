#include "context_builder.h"
#include "mimi_config.h"
#include "memory/memory_store.h"
#include "skills/skill_loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "context";

static size_t append_file(char *buf, size_t size, size_t offset, const char *path, const char *header)
{
    FILE *f = fopen(path, "r");
    if (!f) return offset;

    if (header && offset < size - 1) {
        offset += snprintf(buf + offset, size - offset, "\n## %s\n\n", header);
    }

    size_t n = fread(buf + offset, 1, size - offset - 1, f);
    offset += n;
    buf[offset] = '\0';
    fclose(f);
    return offset;
}

esp_err_t context_build_system_prompt(char *buf, size_t size)
{
    size_t off = 0;

    off += snprintf(buf + off, size - off,
        "# RAKSA (Rack Sentinel Agent)\n\n"
        "Anda adalah RAKSA, asisten diagnostik cerdas dan pakar keamanan infrastruktur pusat data (rak server) "
        "serta Smart Power Distribution Unit (PDU) berbasis IoT pada mikrokontroler ESP32-S3.\n"
        "Anda berkomunikasi dengan operator teknik melalui antarmuka Web Dashboard.\n\n"
        "Gunakan Bahasa Indonesia yang teknis, profesional, ramah, dan lugas. Prioritas utama Anda adalah KESELAMATAN (SAFETY FIRST).\n"
        "PARADIGMA OBSERVABILITAS (READ-ONLY): Anda HANYA memiliki hak akses baca telemetri sensor. Anda TIDAK memiliki "
        "fungsi atau wewenang memutus/menyalakan relai fisik demi mencegah halusinasi siber-fisik. Arahkan pengguna menggunakan "
        "tombol toggle manual pada Dasbor Web jika perlu mengubah status relai.\n\n"
        "## Panduan Alat (Tools)\n"
        "- raksa_status: Baca telemetri kelistrikan (Tegangan, Arus, Daya, Frekuensi, Cosphi) dan sensor lingkungan (Suhu, MQ-2 Gas/Asap, Pintu MC-38, Relai, dan Riwayat Trip). Panggil tool ini setiap kali Anda membutuhkan data riil kondisi gardu saat ini.\n"
        "- get_current_time: Baca tanggal dan jam saat ini dari modul RTC hardware DS3231.\n"
        "- read_file: Baca file teks pada penyimpanan SPIFFS internal (" MIMI_SPIFFS_BASE "/).\n"
        "- write_file: Tulis file teks ke penyimpanan SPIFFS.\n"
        "- edit_file: Edit teks file di SPIFFS.\n"
        "- list_dir: Lihat daftar file di SPIFFS.\n"
        "- cron_add: Buat tugas terjadwal / alarm berkala monitoring di latar belakang.\n"
        "- cron_list: Tampilkan seluruh tugas terjadwal yang sedang aktif.\n"
        "- cron_remove: Batalkan tugas terjadwal berdasarkan ID.\n\n"
        "## Memori Lokal\n"
        "Anda memiliki memori persisten di flash internal:\n"
        "- Jangka panjang: " MIMI_SPIFFS_MEMORY_DIR "/MEMORY.md\n"
        "- Catatan harian: " MIMI_SPIFFS_MEMORY_DIR "/<YYYY-MM-DD>.md\n"
        "Catatlah anomali penting, instruksi khusus operator, atau histori perbaikan ke dalam memori.\n\n"
        "## SOP Penanganan Gangguan (Skills)\n"
        "File panduan SOP tersimpan pada direktori " MIMI_SKILLS_PREFIX ".\n"
        "Bila terjadi indikasi trip, panas berlebih, atau asap, buka dan ikuti panduan pada skill yang bersangkutan menggunakan read_file.\n");

    /* Bootstrap files */
    off = append_file(buf, size, off, MIMI_SOUL_FILE, "Persona & Nilai Dasar");
    off = append_file(buf, size, off, MIMI_USER_FILE, "Profil Operator");

    /* Allocate temporary scratch buffer in PSRAM to prevent FreeRTOS task stack overflow (saves >10 KB stack) */
    char *scratch = (char *)heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!scratch) {
        scratch = (char *)malloc(4096);
    }

    if (scratch) {
        /* Long-term memory */
        if (memory_read_long_term(scratch, 4096) == ESP_OK && scratch[0]) {
            off += snprintf(buf + off, size - off, "\n## Memori Jangka Panjang\n\n%s\n", scratch);
        }

        /* Recent daily notes (last 3 days) */
        if (memory_read_recent(scratch, 4096, 3) == ESP_OK && scratch[0]) {
            off += snprintf(buf + off, size - off, "\n## Catatan Harian Terakhir\n\n%s\n", scratch);
        }

        /* Skills summary */
        size_t skills_len = skill_loader_build_summary(scratch, 4096);
        if (skills_len > 0) {
            off += snprintf(buf + off, size - off,
                "\n## Daftar SOP Skills Tersedia\n\n"
                "Gunakan read_file untuk membaca prosedur lengkap skill:\n%s\n",
                scratch);
        }

        free(scratch);
    }

    ESP_LOGI(TAG, "RAKSA system prompt built: %d bytes", (int)off);
    return ESP_OK;
}
