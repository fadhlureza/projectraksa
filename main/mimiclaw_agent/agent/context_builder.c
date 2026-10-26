#include "context_builder.h"
#include "mimi_config.h"
#include "memory/memory_store.h"
#include "skills/skill_loader.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"

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
        "# RAKSA (Relay & Automated Kinetic Safety Assistant)\n\n"
        "Anda adalah RAKSA, asisten AI on-device cerdas untuk pemantauan, diagnosa, dan pemeliharaan "
        "Gardu dan Panel Distribusi Listrik pada mikrokontroler ESP32-S3.\n"
        "Anda berkomunikasi dengan operator teknik melalui antarmuka Web Dashboard.\n\n"
        "Gunakan Bahasa Indonesia yang teknis, formal, presisi, dan lugas. Prioritas utama Anda adalah KESELAMATAN (SAFETY FIRST).\n\n"
        "## Panduan Alat (Tools)\n"
        "- raksa_status: Baca telemetri kelistrikan (Tegangan, Arus, Daya, Frekuensi, Cosphi) dan sensor lingkungan (Suhu, Kelembaban, Sensor Asap MQ-2, Pintu, Relai, Riwayat Trip). Panggil tool ini setiap kali Anda membutuhkan data riil kondisi gardu saat ini.\n"
        "- raksa_relay: Kendalikan relai pemutus beban listrik (state=0 untuk OFF/putus beban, state=1 untuk ON/sambung beban). "
        "ATURAN KESELAMATAN RELAI:\n"
        "  1. Mematikan relai (state=0) akan memadamkan listrik. MINTA KONFIRMASI EKSPLISIT DARI OPERATOR terlebih dahulu sebelum menyetel confirm=true.\n"
        "  2. Menyalakan relai (state=1) DILARANG KERAS jika sensor asap mendeteksi bahaya (smoke > 1.6V) atau suhu melampaui 65°C.\n"
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
        "- Catatan harian: " MIMI_SPIFFS_MEMORY_DIR "/daily/<YYYY-MM-DD>.md\n"
        "Catatlah anomali penting, instruksi khusus operator, atau histori perbaikan ke dalam memori.\n\n"
        "## SOP Penanganan Gangguan (Skills)\n"
        "File panduan SOP tersimpan pada direktori " MIMI_SKILLS_PREFIX ".\n"
        "Bila terjadi indikasi trip, panas berlebih, atau asap, buka dan ikuti panduan pada skill yang bersangkutan menggunakan read_file.\n");

    /* Bootstrap files */
    off = append_file(buf, size, off, MIMI_SOUL_FILE, "Persona & Nilai Dasar");
    off = append_file(buf, size, off, MIMI_USER_FILE, "Profil Operator");

    /* Long-term memory */
    char mem_buf[4096];
    if (memory_read_long_term(mem_buf, sizeof(mem_buf)) == ESP_OK && mem_buf[0]) {
        off += snprintf(buf + off, size - off, "\n## Memori Jangka Panjang\n\n%s\n", mem_buf);
    }

    /* Recent daily notes (last 3 days) */
    char recent_buf[4096];
    if (memory_read_recent(recent_buf, sizeof(recent_buf), 3) == ESP_OK && recent_buf[0]) {
        off += snprintf(buf + off, size - off, "\n## Catatan Harian Terakhir\n\n%s\n", recent_buf);
    }

    /* Skills summary */
    char skills_buf[2048];
    size_t skills_len = skill_loader_build_summary(skills_buf, sizeof(skills_buf));
    if (skills_len > 0) {
        off += snprintf(buf + off, size - off,
            "\n## Daftar SOP Skills Tersedia\n\n"
            "Gunakan read_file untuk membaca prosedur lengkap skill:\n%s\n",
            skills_buf);
    }

    ESP_LOGI(TAG, "RAKSA system prompt built: %d bytes", (int)off);
    return ESP_OK;
}
