#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "usb_device_uac.h"

static const char *TAG = "i2s_usb";

// === CONFIGURATION ===
#define EXAMPLE_I2S_NUM             0
#define EXAMPLE_AUDIO_SAMPLE_RATE   48000
#define EXAMPLE_AUDIO_BIT_WIDTH     I2S_DATA_BIT_WIDTH_16BIT
#define EXAMPLE_I2S_DMA_DESC_NUM    6
#define EXAMPLE_I2S_DMA_FRAME_NUM   240

#define EXAMPLE_I2S_MCK_IO          GPIO_NUM_0
#define EXAMPLE_I2S_BCK_IO          GPIO_NUM_1
#define EXAMPLE_I2S_WS_IO           GPIO_NUM_2
#define EXAMPLE_I2S_DO_IO           GPIO_NUM_3
#define EXAMPLE_I2S_DI_IO           GPIO_NUM_NC  // not used

// I2S TX handle
static i2s_chan_handle_t i2s_tx_handle = NULL;

static void i2s_driver_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(EXAMPLE_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    chan_cfg.dma_desc_num = EXAMPLE_I2S_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = EXAMPLE_I2S_DMA_FRAME_NUM;

    // Only use TX
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(EXAMPLE_AUDIO_BIT_WIDTH, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = EXAMPLE_I2S_MCK_IO,
            .bclk = EXAMPLE_I2S_BCK_IO,
            .ws   = EXAMPLE_I2S_WS_IO,
            .dout = EXAMPLE_I2S_DO_IO,
            .din  = EXAMPLE_I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_handle));
}

// USB Audio OUT callback: получено от USB → пращаме към I2S
static esp_err_t usb_uac_device_output_cb(uint8_t *buf, size_t len, void *arg)
{
    size_t bytes_written = 0;
    esp_err_t err = i2s_channel_write(i2s_tx_handle, buf, len, &bytes_written, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s write failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// USB Audio IN callback (не се използва)
static esp_err_t usb_uac_device_input_cb(uint8_t *buf, size_t len, size_t *bytes_read, void *arg)
{
    *bytes_read = 0;
    return ESP_OK;
}

// Mute callback (не се използва)
static void usb_uac_device_set_mute_cb(uint32_t mute, void *arg)
{
    ESP_LOGI(TAG, "Mute: %s", mute ? "ON" : "OFF");
}

// Volume callback (не се използва)
static void usb_uac_device_set_volume_cb(uint32_t volume, void *arg)
{
    ESP_LOGI(TAG, "Volume set to: %"PRIu32, volume);
}

// Инициализация на USB аудио
static void usb_uac_device_init(void)
{
    uac_device_config_t config = {
        .output_cb = usb_uac_device_output_cb,
        .input_cb = usb_uac_device_input_cb,
        .set_mute_cb = usb_uac_device_set_mute_cb,
        .set_volume_cb = usb_uac_device_set_volume_cb,
        .cb_ctx = NULL,
    };
    ESP_ERROR_CHECK(uac_device_init(&config));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting I2S USB audio passthrough...");

    // Инициализирай I2S
    i2s_driver_init();

    // Стартирай USB аудио устройство
    usb_uac_device_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
