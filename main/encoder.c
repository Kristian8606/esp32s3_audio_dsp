#include "encoder.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

// Таг за логове
static const char *TAG = "ENCODER";

// Глобални променливи
volatile float volume_s = 0.5; // Начална стойност (50%)
volatile int encoder_position = 50; // Начална позиция (средата)

// Семафор за обработка на прекъсванията
SemaphoreHandle_t encoder_semaphore = NULL;

// Последно време на прекъсване
volatile int64_t last_interrupt_time = 0;

// ISR за енкодера
static void IRAM_ATTR encoder_isr_handler(void* arg) {
    int64_t now = esp_timer_get_time();
    if (now - last_interrupt_time > 20000) { // 5 ms debounce
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(encoder_semaphore, &xHigherPriorityTaskWoken);
        last_interrupt_time = now;

        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

// Инициализация на GPIO и прекъсвания
void init_encoder_interrupts() {
if (encoder_semaphore == NULL) {
    encoder_semaphore = xSemaphoreCreateBinary();
    if (encoder_semaphore == NULL) {
        ESP_LOGE("ENCODER", "Failed to create semaphore!");
        return;
    }
}
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Прекъсвания при промяна
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENCODER_A_PIN) | (1ULL << ENCODER_B_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
    gpio_isr_handler_add(ENCODER_A_PIN, encoder_isr_handler, NULL);
    gpio_isr_handler_add(ENCODER_B_PIN, encoder_isr_handler, NULL);

    ESP_LOGI(TAG, "Encoder interrupts initialized.");
}

// Задача за обработка на енкодера
void encoder_task(void* arg) {
    int last_a_state = gpio_get_level(ENCODER_A_PIN);
    int last_b_state = gpio_get_level(ENCODER_B_PIN);

    while (1) {
        if (xSemaphoreTake(encoder_semaphore, portMAX_DELAY) == pdTRUE) {
            int current_a_state = gpio_get_level(ENCODER_A_PIN);
            int current_b_state = gpio_get_level(ENCODER_B_PIN);

            // Проверка на посоката
            if (current_a_state != last_a_state) {
                if (current_a_state == current_b_state) {
                    encoder_position++;
                } else {
                    encoder_position--;
                }

                // Ограничение на позицията
                if (encoder_position < ENCODER_MIN) {
                    encoder_position = ENCODER_MIN;
                }
                if (encoder_position > ENCODER_MAX) {
                    encoder_position = ENCODER_MAX;
                }

                // Преобразуване в стойност [0.0, 1.0]
                volume_s = (float)encoder_position / 100.0f;

                ESP_LOGI(TAG, "Encoder Position: %d, Volume: %.2f", encoder_position, volume_s);
            }

            // Актуализиране на състоянието
            last_a_state = current_a_state;
            last_b_state = current_b_state;
        }
    }
}