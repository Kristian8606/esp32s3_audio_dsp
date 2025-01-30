#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Глобални променливи
extern volatile float volume_s;
extern volatile int encoder_position;
extern SemaphoreHandle_t encoder_semaphore;

// Минимална и максимална стойност на енкодера
#define ENCODER_MIN 0
#define ENCODER_MAX 100
// GPIO пинове на енкодера
#if CONFIG_DEVICE_1
    #define ENCODER_A_PIN 13
	#define ENCODER_B_PIN 12
	
#elif CONFIG_DEVICE_2
	#define ENCODER_A_PIN 3
	#define ENCODER_B_PIN 4
	
#elif CONFIG_DEVICE_3
	#define ENCODER_A_PIN 3
	#define ENCODER_B_PIN 4

#elif CONFIG_DEVICE_4
	#define ENCODER_A_PIN 10
	#define ENCODER_B_PIN 9

#endif


// Деклариране на функции
void init_encoder_interrupts();
void encoder_task(void* arg);

#endif // ENCODER_H