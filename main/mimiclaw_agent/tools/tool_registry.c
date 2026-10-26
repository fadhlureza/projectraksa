#include "tool_registry.h"
#include "mimi_config.h"
#include "tools/tool_raksa.h"
#include "tools/tool_get_time.h"
#include "tools/tool_files.h"
#include "tools/tool_cron.h"

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "tools";

#define MAX_TOOLS 16

static mimi_tool_t s_tools[MAX_TOOLS];
static int s_tool_count = 0;
static char *s_tools_json = NULL;  /* cached JSON array string */

static void register_tool(const mimi_tool_t *tool)
{
    if (s_tool_count >= MAX_TOOLS) {
        ESP_LOGE(TAG, "Tool registry full");
        return;
    }
    s_tools[s_tool_count++] = *tool;
    ESP_LOGI(TAG, "Registered tool: %s", tool->name);
}

static void build_tools_json(void)
{
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < s_tool_count; i++) {
        cJSON *tool = cJSON_CreateObject();
        cJSON_AddStringToObject(tool, "name", s_tools[i].name);
        cJSON_AddStringToObject(tool, "description", s_tools[i].description);

        cJSON *schema = cJSON_Parse(s_tools[i].input_schema_json);
        if (schema) {
            cJSON_AddItemToObject(tool, "input_schema", schema);
        }

        cJSON_AddItemToArray(arr, tool);
    }

    free(s_tools_json);
    s_tools_json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    ESP_LOGI(TAG, "Tools JSON built (%d tools registered)", s_tool_count);
}

esp_err_t tool_registry_init(void)
{
    s_tool_count = 0;

    /* 1. raksa_status: Electrical and environmental telemetry */
    mimi_tool_t rs = {
        .name = "raksa_status",
        .description = "Dapatkan telemetri kelistrikan dan lingkungan terkini dari panel distribusi RAKSA (tegangan, arus, daya, energi, frekuensi, power factor, suhu, kelembaban, asap, status pintu, relai, dan riwayat trip).",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_raksa_status_execute,
    };
    register_tool(&rs);

    /* 2. get_current_time: DS3231 RTC */
    tool_get_time_init();
    mimi_tool_t gt = {
        .name = "get_current_time",
        .description = "Baca waktu dan tanggal saat ini dari modul RTC hardware DS3231. Gunakan saat butuh mengetahui jam sekarang atau sebelum menjadwalkan tugas berkala.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_get_time_execute,
    };
    register_tool(&gt);

    /* 4. read_file */
    mimi_tool_t rf = {
        .name = "read_file",
        .description = "Baca isi file teks dari partisi penyimpanan internal SPIFFS (misal: SOP skill di /spiffs/skills/ atau konfigurasi).",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Path absolut file berawalan " MIMI_SPIFFS_BASE "/\"}},"
            "\"required\":[\"path\"]}",
        .execute = tool_read_file_execute,
    };
    register_tool(&rf);

    /* 5. write_file */
    mimi_tool_t wf = {
        .name = "write_file",
        .description = "Tulis atau perbarui file pada partisi penyimpanan internal SPIFFS.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Path absolut file berawalan " MIMI_SPIFFS_BASE "/\"},"
            "\"content\":{\"type\":\"string\",\"description\":\"Konten teks file yang akan ditulis\"}},"
            "\"required\":[\"path\",\"content\"]}",
        .execute = tool_write_file_execute,
    };
    register_tool(&wf);

    /* 6. edit_file */
    mimi_tool_t ef = {
        .name = "edit_file",
        .description = "Ganti bagian teks tertentu dalam file teks di SPIFFS.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Path absolut file berawalan " MIMI_SPIFFS_BASE "/\"},"
            "\"old_string\":{\"type\":\"string\",\"description\":\"Teks yang ingin diganti\"},"
            "\"new_string\":{\"type\":\"string\",\"description\":\"Teks pengganti baru\"}},"
            "\"required\":[\"path\",\"old_string\",\"new_string\"]}",
        .execute = tool_edit_file_execute,
    };
    register_tool(&ef);

    /* 7. list_dir */
    mimi_tool_t ld = {
        .name = "list_dir",
        .description = "Daftar file di partisi SPIFFS, opsional dengan filter prefix (contoh: " MIMI_SPIFFS_BASE "/skills/).",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"prefix\":{\"type\":\"string\",\"description\":\"Filter awalan path, misal: " MIMI_SPIFFS_BASE "/skills/\"}},"
            "\"required\":[]}",
        .execute = tool_list_dir_execute,
    };
    register_tool(&ld);

    /* 8. cron_add */
    mimi_tool_t ca = {
        .name = "cron_add",
        .description = "Jadwalkan tugas monitoring berkala atau alarm terjadwal yang akan mengeksekusi prompt AI otomatis di latar belakang.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{"
            "\"name\":{\"type\":\"string\",\"description\":\"Nama singkat tugas\"},"
            "\"schedule_type\":{\"type\":\"string\",\"description\":\"'every' untuk interval berulang (detik) atau 'at' untuk satu kali pada unix epoch\"},"
            "\"interval_s\":{\"type\":\"integer\",\"description\":\"Interval dalam detik jika schedule_type='every'\"},"
            "\"at_epoch\":{\"type\":\"integer\",\"description\":\"Timestamp unix target jika schedule_type='at'\"},"
            "\"message\":{\"type\":\"string\",\"description\":\"Pesan instruksi yang dikirim ke AI saat tugas aktif\"},"
            "\"channel\":{\"type\":\"string\",\"description\":\"Channel balasan (default 'web' atau 'system')\"},"
            "\"chat_id\":{\"type\":\"string\",\"description\":\"ID sesi obrolan penerima hasil tugas\"}"
            "},"
            "\"required\":[\"name\",\"schedule_type\",\"message\"]}",
        .execute = tool_cron_add_execute,
    };
    register_tool(&ca);

    /* 9. cron_list */
    mimi_tool_t cl = {
        .name = "cron_list",
        .description = "Daftar seluruh tugas cron/terjadwal yang sedang aktif di sistem RAKSA.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_cron_list_execute,
    };
    register_tool(&cl);

    /* 10. cron_remove */
    mimi_tool_t cr = {
        .name = "cron_remove",
        .description = "Hapus tugas cron/terjadwal berdasarkan 8-karakter ID tugas.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"job_id\":{\"type\":\"string\",\"description\":\"8-karakter ID tugas yang ingin dihapus\"}},"
            "\"required\":[\"job_id\"]}",
        .execute = tool_cron_remove_execute,
    };
    register_tool(&cr);

    build_tools_json();

    ESP_LOGI(TAG, "RAKSA Tool registry initialized successfully (%d tools)", s_tool_count);
    return ESP_OK;
}

const char *tool_registry_get_tools_json(void)
{
    return s_tools_json;
}

esp_err_t tool_registry_execute(const char *name, const char *input_json,
                                char *output, size_t output_size)
{
    for (int i = 0; i < s_tool_count; i++) {
        if (strcmp(s_tools[i].name, name) == 0) {
            ESP_LOGI(TAG, "Executing tool: %s", name);
            return s_tools[i].execute(input_json, output, output_size);
        }
    }

    ESP_LOGW(TAG, "Unknown tool: %s", name);
    snprintf(output, output_size, "Error: Tool tidak dikenal '%s'", name);
    return ESP_ERR_NOT_FOUND;
}
