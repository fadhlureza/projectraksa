#include "pzem.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "PZEM";

#define PZEM_RX_BUF_SIZE 128
#define PZEM_TX_BUF_SIZE 0

// Modbus RTU Read Input Registers Command for PZEM-004T V3.0
// Slave ID: 0x01, Function: 0x04, Start Addr: 0x0000, Num Regs: 0x000A (10 registers)
// Pre-calculated CRC: 0x70 0x0D (Low, High)
static const uint8_t pzem_read_cmd[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x70, 0x0D};

// Standard Modbus CRC-16 calculation
static uint16_t crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

esp_err_t pzem_init(uart_port_t uart_num, gpio_num_t tx_pin, gpio_num_t rx_pin) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if (err != ESP_OK) return err;

    err = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    err = uart_driver_install(uart_num, PZEM_RX_BUF_SIZE * 2, PZEM_TX_BUF_SIZE, 0, NULL, 0);
    return err;
}

esp_err_t pzem_read_data(uart_port_t uart_num, pzem_data_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;

    // Clear RX buffer before sending request
    uart_flush_input(uart_num);

    // Send read command
    int txBytes = uart_write_bytes(uart_num, (const char *)pzem_read_cmd, sizeof(pzem_read_cmd));
    if (txBytes != sizeof(pzem_read_cmd)) {
        ESP_LOGE(TAG, "Failed to send read command");
        return ESP_FAIL;
    }

    // Expected response size: 
    // ID(1) + Func(1) + BytesCount(1) + Data(20) + CRC(2) = 25 bytes
    uint8_t rx_buf[32];
    int rxBytes = uart_read_bytes(uart_num, rx_buf, 25, pdMS_TO_TICKS(1000));

    if (rxBytes < 25) {
        ESP_LOGW(TAG, "Timeout or incomplete data (received %d bytes)", rxBytes);
        return ESP_ERR_TIMEOUT;
    }

    // Validate Slave ID and Function Code
    if (rx_buf[0] != 0x01 || rx_buf[1] != 0x04) {
        ESP_LOGE(TAG, "Invalid response header: %02X %02X", rx_buf[0], rx_buf[1]);
        return ESP_FAIL;
    }

    // Validate CRC
    uint16_t calculated_crc = crc16(rx_buf, 23); // CRC over first 23 bytes
    uint16_t received_crc = rx_buf[23] | (rx_buf[24] << 8);
    if (calculated_crc != received_crc) {
        ESP_LOGE(TAG, "CRC Error: expected %04X, got %04X", calculated_crc, received_crc);
        return ESP_ERR_INVALID_CRC;
    }

    // Parse data registers (Big Endian)
    // 0x00 Voltage (0.1 V)
    uint16_t raw_voltage = (rx_buf[3] << 8) | rx_buf[4];
    data->voltage = raw_voltage / 10.0f;

    // 0x01-0x02 Current (0.001 A) - Low 16 bits then High 16 bits
    uint16_t current_l = (rx_buf[5] << 8) | rx_buf[6];
    uint16_t current_h = (rx_buf[7] << 8) | rx_buf[8];
    uint32_t raw_current = (current_h << 16) | current_l;
    data->current = raw_current / 1000.0f;

    // 0x03-0x04 Power (0.1 W)
    uint16_t power_l = (rx_buf[9] << 8) | rx_buf[10];
    uint16_t power_h = (rx_buf[11] << 8) | rx_buf[12];
    uint32_t raw_power = (power_h << 16) | power_l;
    data->power = raw_power / 10.0f;

    // 0x05-0x06 Energy (1 Wh)
    uint16_t energy_l = (rx_buf[13] << 8) | rx_buf[14];
    uint16_t energy_h = (rx_buf[15] << 8) | rx_buf[16];
    uint32_t raw_energy = (energy_h << 16) | energy_l;
    data->energy = raw_energy;

    // 0x07 Frequency (0.1 Hz)
    uint16_t raw_freq = (rx_buf[17] << 8) | rx_buf[18];
    data->frequency = raw_freq / 10.0f;

    // 0x08 Power Factor (0.01)
    uint16_t raw_pf = (rx_buf[19] << 8) | rx_buf[20];
    data->power_factor = raw_pf / 100.0f;

    // 0x09 Alarms
    data->alarms = (rx_buf[21] << 8) | rx_buf[22];

    return ESP_OK;
}
