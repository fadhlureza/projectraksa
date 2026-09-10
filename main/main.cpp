#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

// System configuration and hardware constants
#include "constant.h"

// Sensor drivers
#include "door/mc38.h"
#include "smoke/mq2.h"
#include "temperature/temperature.h"

// Network & Web Server Modules
#include "network.h"
#include "webserver.h"

static const char *TAG = "RAKSA_MAIN";

// Structure to store shared system telemetry data
typedef struct {
  bool door_open;
  int smoke_raw;
  float temperature;

  // Future power monitoring metrics (PZEM-004T)
  float voltage;
  float current;
} SystemData_t;

// Global variables
SystemData_t rack_data;
SemaphoreHandle_t data_mutex;

// Sensor hardware handles
adc_oneshot_unit_handle_t mq2_handle;
onewire_bus_handle_t ow_bus;
ds18b20_device_handle_t ds18b20_dev;

/**
 * @brief FreeRTOS task for continuous sensor data acquisition.
 */
void sensor_task(void *pvParameters) {
  while (1) {
    // Read sensor raw values
    bool is_open = mc38_is_door_open(PIN_DOOR_SENSOR);
    int smoke_val = mq2_read_raw(mq2_handle, ADC_CHANNEL_8);
    float temp_val = ds18b20_read_temp(ds18b20_dev);

    // Thread-safe update of shared system state
    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
      rack_data.door_open = is_open;
      rack_data.smoke_raw = smoke_val;
      rack_data.temperature = temp_val;

      xSemaphoreGive(data_mutex);
    }

    // Log telemetry metrics for debugging
    ESP_LOGI(TAG, "Door: %s | Smoke: %d | Temp: %.2f C",
             is_open ? "OPEN" : "CLOSED", smoke_val, temp_val);

    // Yield task execution to reset the watchdog timer
    vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_RATE_MS));
  }
}

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

  // Create mutex for thread-safe access to system data
  data_mutex = xSemaphoreCreateMutex();

  // Initialize sensor peripherals
  ESP_LOGI(TAG, ">> Initializing sensors...");
  mc38_init(PIN_DOOR_SENSOR);
  mq2_init(ADC_CHANNEL_8, &mq2_handle);
  ds18b20_init_sensor(PIN_TEMP_SENSOR, &ow_bus, &ds18b20_dev);

  // Create sensor reading task pinned to Core 0
  xTaskCreatePinnedToCore(sensor_task,   // Task function
                          "SensorTask",  // Task name
                          4096,          // Stack size in bytes
                          NULL,          // Task parameter
                          5,             // Task priority
                          NULL,          // Task handle
                          0              // Core ID (Core 0)
  );
  ESP_LOGI(TAG, ">> Sensor task successfully started on Core 0");

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