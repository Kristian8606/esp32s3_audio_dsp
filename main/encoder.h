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
    #define ENCODER_A_PIN 13
	#define ENCODER_B_PIN 12
	



// Деклариране на функции
void init_encoder_interrupts();
void encoder_task(void* arg);

#endif // ENCODER_H