#include "fir_filter.h"
#include "fir_coeffs.h"
#include "webserver.h"


#if CONFIG_FILTER_FIR_IIR || CONFIG_FILTER_FIR
// FIR коефициенти и линии на закъснение



static __attribute__((aligned(16))) float delay_line_left[FIR_COEFFS_LEN + 4];
static __attribute__((aligned(16))) float delay_line_right[FIR_COEFFS_LEN + 4];

// Входни и изходни буфери
__attribute__((aligned(16))) float fir_input_left[BUFFER_SIZE / 2];
__attribute__((aligned(16))) float fir_input_right[BUFFER_SIZE / 2];
__attribute__((aligned(16))) float fir_output_left[BUFFER_SIZE / 2];
__attribute__((aligned(16))) float fir_output_right[BUFFER_SIZE / 2];

// FIR обекти
static fir_f32_t fir_left;
static fir_f32_t fir_right;

// Инициализация на FIR филтъра
void fir_filter_init(void) {
    dsps_fir_init_f32(&fir_left, coeffs, delay_line_left, FIR_COEFFS_LEN);
    dsps_fir_init_f32(&fir_right, coeffs, delay_line_right, FIR_COEFFS_LEN);
    ESP_LOGI("FIR", "FIR filter initialized with coeff len: %d", FIR_COEFFS_LEN);
}

// Задача за обработка на левия канал
void process_left(void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        dsps_fir_f32_aes3(&fir_left, fir_input_left, fir_output_left, BUFFER_SIZE / 2);
        xTaskNotifyGive(main_task_handle);
    }
}

// Задача за обработка на десния канал
void process_right(void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        dsps_fir_f32_aes3(&fir_right, fir_input_right, fir_output_right, BUFFER_SIZE / 2);
        xTaskNotifyGive(main_task_handle);
    }
}

#endif