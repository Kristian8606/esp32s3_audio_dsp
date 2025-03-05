#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

static const char *TAG = "WM8805";

// I2C конфигурация
#define I2C_MASTER_SCL_IO           6       // GPIO за SCL
#define I2C_MASTER_SDA_IO           7       // GPIO за SDA
#define I2C_MASTER_NUM              I2C_NUM_0                   // I2C порт
#define I2C_MASTER_FREQ_HZ          100000 // I2C честота
#define I2C_MASTER_TIMEOUT_MS       1000

// WM8805 адрес (0x3A)
#define WM8805_ADDR                 0x3B 

// GPIO за RESET
#define WM8805_RESET_GPIO           2 // Промени според твоя хардуер

// I2C хендъли
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t wm8805_handle;

/**
 * @brief Четене от регистър на WM8805
 */
static esp_err_t wm8805_read_register(uint8_t reg, uint8_t *data) {
    return i2c_master_transmit_receive(wm8805_handle, &reg, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Запис в регистър на WM8805
 */
static esp_err_t wm8805_write_register(uint8_t reg, uint8_t value) {
    uint8_t write_buf[2] = {reg, value};
    return i2c_master_transmit(wm8805_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Инициализация на I2C
 */
static void i2c_master_init() {
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
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &wm8805_handle));
}

/**
 * @brief Инициализация на WM8805
 */
void wm8805_init() {
    // Инициализация на RESET пина
    gpio_set_direction(WM8805_RESET_GPIO, GPIO_MODE_OUTPUT);

    // Хардуерен RESET
    gpio_set_level(WM8805_RESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(WM8805_RESET_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Проверка на ID
    uint8_t id1, id0, rev;
    wm8805_read_register(1, &id1);
    wm8805_read_register(0, &id0);
    wm8805_read_register(2, &rev);
    ESP_LOGI(TAG, "WM8805 ID: 0x%02X%02X Rev: 0x%02X", id1, id0, rev);

    // Основна конфигурация
  //  wm8805_write_register(8, 0x30);  // Disable CLKOUT
    wm8805_write_register(10, 0x7E); // Mask interrupts
   // wm8805_write_register(27, 0x0A); // TX Audio I/F I2S, 24-bit
    wm8805_write_register(28, 0xCA); // RX Audio I/F I2S, 24-bit
  //	wm8805_write_register(0x07, 0x04);  // PLL Fractional Enable
  		wm8805_write_register(0x07, 0x04);  // Use XO as clock source

  //	wm8805_write_register(0x07, 0x02);  // BCLK = 64 x fs
wm8805_write_register(0x06, 0x07);  // PLL_N = 7
wm8805_write_register(0x05, 0x36);  // PLL_CTRL2
wm8805_write_register(0x04, 0xFD);  // PLL_CTRL1
wm8805_write_register(0x03, 0x21);  // PLL_CTRL0


//wm8805_write_register(27, 0x0A); // TX Audio I/F: I2S, 24-bit
wm8805_write_register(28, 0xCA); // RX Audio I/F: I2S, 24-bit
//wm8805_write_register(8, 0x1B);  // Избор на вход SPDIF RX4 (TOSLINK)
wm8805_write_register(0x08, 0x10); // Избира SPDIF RX0
wm8805_write_register(30, 0x00); // Включване на захранването
	//wm8805_write_register(0x27, 0x0A);  // TX Audio I/F: I2S, 24-bit
	wm8805_write_register(0x09, 0x08);  // CLKOUT = MCLK = 256x fs
	wm8805_write_register(0x1E, 0xC0);  // Enable XO (crystal oscillator)
	    wm8805_write_register(0x08, 0x00);  // Fill Mode = Zero Fill (запълване с нули при липса на сигнал)

    // Включване на захранването
 //   wm8805_write_register(30, 0);
  //  wm8805_write_register(0x03, 0x00);  // Disable PLL, use external MCLK not worck

    ESP_LOGI(TAG, "WM8805 инициализиран успешно.");
}

/**
 * @brief Мониторинг на SPDIF честотата
 */
void wm8805_task(void *arg) {
    while (1) {
        uint8_t status;
        wm8805_read_register(0x0C, &status);
         uint8_t spdif_status;
		wm8805_read_register(0x0D, &spdif_status);
		uint8_t sr1, sr2;
		wm8805_read_register(0x0E, &sr1);
		wm8805_read_register(0x0F, &sr2);
		//uint8_t status;
		//wm8805_read_register(0x0C, &status);
		if (status & 0x40) {
		    ESP_LOGI(TAG, "PLL locked to RX0");
		} else {
		    ESP_LOGW(TAG, "No lock on RX0");
		}
		uint16_t sample_rate = (sr1 << 8) | sr2;
		ESP_LOGI(TAG, "SPDIF Sample Rate: %d Hz", sample_rate);

        uint8_t sample_rate_code = (status >> 4) & 0x03;
        const char *sample_rate_str = "Неизвестна честота";

        switch (sample_rate_code) {
            case 3: sample_rate_str = "32 kHz"; break;
            case 2: sample_rate_str = "44.1/48 kHz"; break;
            case 1: sample_rate_str = "88.2/96 kHz"; break;
            case 0: sample_rate_str = "176.4/192 kHz"; break;
        }

        ESP_LOGI(TAG, "SPDIF честота: %s", sample_rate_str);
         ESP_LOGI(TAG, "SPDIF Стасус: 0x%02X", status);
        
		ESP_LOGI(TAG, "SPDIF Channel Status: 0x%02X", spdif_status);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Основна функция
 */
void app_main() {
    // Инициализация на I2C
    i2c_master_init();

    // Инициализация на WM8805
    wm8805_init();

    // Стартиране на FreeRTOS задача за мониторинг
    xTaskCreate(wm8805_task, "wm8805_task", 4096, NULL, 5, NULL);
}
