#include "esp_log.h"
#include "nvs_flash.h"

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

  // Initialize RTC DS3231 and synchronize system time
  ESP_LOGI(TAG, ">> Initializing RTC DS3231...");
  rtc_init(PIN_I2C_SDA, PIN_I2C_SCL);

  // Initialize MicroSD Logger (Graceful degradation if no card inserted)
  ESP_LOGI(TAG, ">> Initializing MicroSD Logger...");
  sdlog_init();

  // Initialize sensors and start polling task
  ESP_LOGI(TAG, ">> Initializing Sensor Manager...");
  SensorManager::init();
  SensorManager::start_task();

  // Initialize Network Manager
  ESP_LOGI(TAG, ">> Initializing Network Manager...");
  ESP_ERROR_CHECK(NetworkManager::init());
  
  // Start Access Point (AP) mode
  // Password must be at least 8 characters
  ESP_ERROR_CHECK(NetworkManager::start_ap_sta_mode("RAKSA_WIFI", "raksa123"));

  // Start Web Server for Dashboard UI
  ESP_LOGI(TAG, ">> Starting Web Server...");
  ESP_ERROR_CHECK(WebServer::start());

  ESP_LOGI(TAG, "=== RAKSA SYSTEM FULLY OPERATIONAL ===");
  ESP_LOGI(TAG, ">> Connect to Wi-Fi SSID 'RAKSA_WIFI' and visit http://192.168.4.1");
}