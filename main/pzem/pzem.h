#pragma once

#include "driver/uart.h"
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// Data structure to hold all PZEM-004T metrics
typedef struct {
    float voltage;       // V
    float current;       // A
    float power;         // W
    float energy;        // Wh
    float frequency;     // Hz
    float power_factor;  // 0.0 - 1.0
    uint16_t alarms;     // Alarm status
} pzem_data_t;

/**
 * @brief Initialize UART for PZEM-004T communication
 *
 * @param uart_num The UART port to use (e.g., UART_NUM_1)
 * @param tx_pin GPIO pin for TX
 * @param rx_pin GPIO pin for RX
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pzem_init(uart_port_t uart_num, gpio_num_t tx_pin, gpio_num_t rx_pin);

/**
 * @brief Read data from the PZEM-004T sensor
 *
 * @param uart_num The UART port the sensor is connected to
 * @param data Pointer to store the read data
 * @return esp_err_t ESP_OK on success, or error code
 */
esp_err_t pzem_read_data(uart_port_t uart_num, pzem_data_t *data);

#ifdef __cplusplus
}
#endif
