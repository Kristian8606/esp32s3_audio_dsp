/************************************************************************************

WM8805 S/PDIF Receiver (ESP-IDF 5.4 Adaptation)

Based on: http://www.itsonlyaudio.com/audio-hardware/arduino-software-control-for-wm8804wm8805/

v0.83 : - Updated to use ESP-IDF 5.4 I2C Master API.
        - Uses i2c_master_bus_handle_t and i2c_master_dev_handle_t.
        - Outputs detected sampling rate.
        - Input switching only through editing the code.
        - Periodically checks the status register.

************************************************************************************/

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

static const char *TAG = "WM8805";

#define I2C_MASTER_SCL_IO           22    // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           21    // GPIO number for I2C master data
#define I2C_MASTER_NUM              I2C_NUM_0  // I2C port number for master
#define I2C_MASTER_FREQ_HZ          100000  // I2C master clock frequency
#define I2C_MASTER_TIMEOUT_MS       1000
#define WM8805_ADDR                 0x3A  // Device address

// Handles for I2C bus and device
i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

// Function to initialize the I2C master bus and device
void i2c_master_init() {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = WM8805_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}

// Function to write a byte of data to a register
esp_err_t i2c_write_register(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

// Function to read a byte of data from a register
esp_err_t i2c_read_register(uint8_t reg, uint8_t *data) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

// Function to initialize the WM8805 chip
void wm8805_init() {
    // Configure GPIO2 as an output and set it high (could be used for reset or enable)
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Read and print the device ID to confirm communication
    uint8_t device_id;
    if (i2c_read_register(1, &device_id) == ESP_OK) {
        ESP_LOGI(TAG, "WM8805 found, Device ID: %02X", device_id);
    } else {
        ESP_LOGE(TAG, "No response from WM8805!");
    }
    
    // Configuration sequence for WM8805
    i2c_write_register(0, 0); // Reset device
    i2c_write_register(7, 0x04); // Enable PLL
    i2c_write_register(8, 0x30); // Configure PLL settings
    i2c_write_register(10, 126); // Configure PLL Divider
    i2c_write_register(27, 0x0A); // Set Output Format
    i2c_write_register(28, 0xCA); // Set Clock Routing
    i2c_write_register(6, 7);  // Enable S/PDIF receiver
    i2c_write_register(5, 0x36); // Configure receiver settings
    i2c_write_register(4, 0xFD); // Enable MCLK output
    i2c_write_register(3, 0x21); // Configure other parameters
    i2c_write_register(9, 0);   // Enable outputs
    i2c_write_register(30, 0);  // Unmute outputs
    i2c_write_register(8, 0x1B); // Final configuration
}

// Task to monitor S/PDIF status periodically
void wm8805_monitor_task(void *arg) {
    uint8_t status;
    while (1) {
        if (i2c_read_register(12, &status) == ESP_OK) {
            ESP_LOGI(TAG, "S/PDIF Status: %02X", status);
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Check status every 500ms
    }
}

// Main function
void app_main() {
    i2c_master_init(); // Initialize I2C bus and device
    wm8805_init(); // Initialize WM8805 receiver
    xTaskCreate(wm8805_monitor_task, "wm8805_monitor_task", 2048, NULL, 5, NULL); // Start monitoring task
}
