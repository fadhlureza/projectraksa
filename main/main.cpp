#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

// System configuration and hardware constants
#include "constant.h"

// Sensor Manager
#include "sensor_manager/sensor.h"

// Hardware Storage & Clock
#include "rtc/rtc.h"
#include "sdcard/sdlog.h"

// Network & Web Server Modules
#include "network.h"
#include "webserver.h"

// MimiClaw AI Agent
#include "mimiclaw_agent/mimi_agent.h"

static const char *TAG = "RAKSA_MAIN";

extern "C" void app_main(void) {
  // Initialize Non-Volatile Storage (NVS) required by Wi-Fi and system config
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "=== RAKSA SYSTEM BOOTING ===");

  // 1. Initialize RTC DS3231 and synchronize system time
  ESP_LOGI(TAG, ">> Initializing RTC DS3231...");
  ds3231_rtc_init(PIN_I2C_SDA, PIN_I2C_SCL);

  // 2. Initialize MicroSD Logger (Graceful degradation if no card inserted)
  ESP_LOGI(TAG, ">> Initializing MicroSD Logger...");
  sdlog_init();

  // 3. Mount internal Flash SPIFFS (Persona, SOP Skills, & Persistent Memory)
  ESP_LOGI(TAG, ">> Mounting SPIFFS partition...");
  esp_vfs_spiffs_conf_t spiffs_conf = {
      .base_path = "/spiffs",
      .partition_label = "spiffs",
      .max_files = 16,
      .format_if_mount_failed = false,
  };
  esp_err_t spiffs_ret = esp_vfs_spiffs_register(&spiffs_conf);
  if (spiffs_ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(spiffs_ret));
  } else {
      size_t total = 0, used = 0;
      esp_spiffs_info("spiffs", &total, &used);
      ESP_LOGI(TAG, "SPIFFS mounted successfully: total %d KB, used %d KB", (int)(total / 1024), (int)(used / 1024));
  }

  // 4. Initialize sensors and start background polling task
  ESP_LOGI(TAG, ">> Initializing Sensor Manager...");
  SensorManager::init();
  SensorManager::start_task();

  // 5. Initialize Network Manager and start AP + STA mode
  ESP_LOGI(TAG, ">> Initializing Network Manager...");
  ESP_ERROR_CHECK(NetworkManager::init());
  
  // Start Access Point (AP) mode
  ESP_ERROR_CHECK(NetworkManager::start_ap_sta_mode("RAKSA_WIFI", "raksa123"));

  // 6. Start Web Server (Dashboard UI + /ws/chat WebSocket endpoint)
  ESP_LOGI(TAG, ">> Starting Web Server...");
  ESP_ERROR_CHECK(WebServer::start());

  // 7. Initialize and start MimiClaw on-device AI Agent
  ESP_LOGI(TAG, ">> Initializing MimiClaw AI Agent...");
  esp_err_t agent_ret = mimi_agent_init();
  if (agent_ret == ESP_OK) {
      ESP_LOGI(TAG, ">> Starting MimiClaw AI Agent background tasks...");
      mimi_agent_start();
  } else {
      ESP_LOGE(TAG, "MimiClaw Agent init failed: %s", esp_err_to_name(agent_ret));
  }

  ESP_LOGI(TAG, "=== RAKSA SYSTEM FULLY OPERATIONAL ===");
  ESP_LOGI(TAG, ">> Connect to Wi-Fi SSID 'RAKSA_WIFI' and visit http://192.168.4.1");
}