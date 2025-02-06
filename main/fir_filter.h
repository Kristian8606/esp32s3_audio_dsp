#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <stddef.h>
#include "esp_dsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUFFER_SIZE    2048
#define FIR_COEFFS_LEN 1024


#if CONFIG_FILTER_FIR_IIR || CONFIG_FILTER_FIR
// Константи за FIR филтъра


// Хендъли за задачите
extern TaskHandle_t left_task_handle;
extern TaskHandle_t right_task_handle;
extern TaskHandle_t main_task_handle;

// Глобални променливи за FIR филтъра
extern __attribute__((aligned(16))) float fir_input_left[BUFFER_SIZE / 2];
extern __attribute__((aligned(16))) float fir_input_right[BUFFER_SIZE / 2];
extern __attribute__((aligned(16))) float fir_output_left[BUFFER_SIZE / 2];
extern __attribute__((aligned(16))) float fir_output_right[BUFFER_SIZE / 2];

// Прототипи на функции
void fir_filter_init(void);
void process_left(void *arg);
void process_right(void *arg);

#endif //#if CONFIG_FILTER_FIR_IIR || CONFIG_FILTER_FIR

#endif // FIR_FILTER_H