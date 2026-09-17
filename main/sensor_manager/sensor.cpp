#include "sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// Sensor drivers
#include "door/mc38.h"
#include "smoke/mq2.h"
#include "temperature/temperature.h"
#include "pzem/pzem.h"
#include "battery/battery.h"

namespace SensorManager {

static const char *TAG = "SENSOR_MGR";

// Global protected state
static SystemData_t current_data;
static SemaphoreHandle_t data_mutex = nullptr;

// Sensor hardware handles
static adc_oneshot_unit_handle_t adc1_handle = nullptr;
static onewire_bus_handle_t ow_bus = nullptr;
static ds18b20_device_handle_t ds18b20_dev = nullptr;

static void sensor_task(void *pvParameters) {
    TickType_t last_temp_trigger = 0;
    bool temp_converting = false;
    float temp_val = -999.0;

    while (1) {
        // Read sensor raw values
        bool is_open = mc38_is_door_open(PIN_DOOR_SENSOR);
        float smoke_v = mq2_read_voltage(adc1_handle, ADC_CHANNEL_5);
        
        // Read Battery
        float batt_v = battery_read_voltage(adc1_handle, ADC_CHANNEL_3);
        int batt_pct = battery_get_percentage(batt_v);
        
        // Non-blocking DS18B20 handling
        if (!temp_converting) {
            ds18b20_trigger(ds18b20_dev);
            last_temp_trigger = xTaskGetTickCount();
            temp_converting = true;
        } else {
            if (xTaskGetTickCount() - last_temp_trigger >= pdMS_TO_TICKS(800)) {
                temp_val = ds18b20_get(ds18b20_dev);
                temp_converting = false;
            }
        }

        // Read PZEM-004T
        pzem_data_t p_data;
        bool pzem_ok = (pzem_read_data(UART_NUM_1, &p_data) == ESP_OK);

        // Thread-safe update of shared system state
        if (data_mutex != nullptr && xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
            current_data.door_open = is_open;
            current_data.smoke_voltage = smoke_v;
            current_data.temperature = temp_val;
            current_data.battery_voltage = batt_v;
            current_data.battery_percentage = batt_pct;

            if (pzem_ok) {
                current_data.voltage = p_data.voltage;
                current_data.current = p_data.current;
                current_data.power = p_data.power;
                current_data.energy = p_data.energy;
                current_data.frequency = p_data.frequency;
                current_data.power_factor = p_data.power_factor;
            }

            xSemaphoreGive(data_mutex);
        }

        // Log telemetry metrics for debugging
        ESP_LOGI(TAG, "Door: %s | Smoke: %.2fV | Temp: %.2f C | Batt: %.2fV (%d%%)",
                 is_open ? "OPEN" : "CLOSED", smoke_v, temp_val, batt_v, batt_pct);
        if (pzem_ok) {
            ESP_LOGI(TAG, "PZEM -> V:%.1f I:%.3f P:%.1f E:%.0f Hz:%.1f PF:%.2f",
                     p_data.voltage, p_data.current, p_data.power,
                     p_data.energy, p_data.frequency, p_data.power_factor);
        }

        // Yield task execution to reset the watchdog timer
        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_RATE_MS));
    }
}

void init() {
    ESP_LOGI(TAG, ">> Initializing SensorManager...");
    
    // Create mutex
    data_mutex = xSemaphoreCreateMutex();

    // Initialize ADC Unit 1
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // Initialize sensors
    mc38_init(PIN_DOOR_SENSOR);
    mq2_init(adc1_handle, ADC_CHANNEL_5);
    battery_init(adc1_handle, ADC_CHANNEL_3);
    ds18b20_init_sensor(PIN_TEMP_SENSOR, &ow_bus, &ds18b20_dev);
    
    if (pzem_init(UART_NUM_1, PIN_PZEM_TX, PIN_PZEM_RX) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PZEM UART");
    } else {
        ESP_LOGI(TAG, "PZEM UART initialized on UART1");
    }
}

void start_task() {
    xTaskCreatePinnedToCore(sensor_task,
                            "SensorTask",
                            4096,
                            NULL,
                            5,
                            NULL,
                            0); // Core 0
    ESP_LOGI(TAG, ">> Sensor task started on Core 0");
}

SystemData_t get_data() {
    SystemData_t copy = {};
    if (data_mutex != nullptr && xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
        copy = current_data;
        xSemaphoreGive(data_mutex);
    }
    return copy;
}

} // namespace SensorManager
