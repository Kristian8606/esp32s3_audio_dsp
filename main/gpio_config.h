#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

    // Проверка на избраното устройство
#if CONFIG_DEVICE_1
    // Дефиниране на GPIO пинове
	#define I2S_PIN_BCLK    8 //6  // BCLK
	#define I2S_PIN_WS      7 //5  // WS
	#define I2S_PIN_DIN     6 //7  // DIN вход
	#define I2S_PIN_DOUT    5 //4  // DOUT изход

#elif CONFIG_DEVICE_2//
	#define I2S_PIN_BCLK    8 //7
	#define I2S_PIN_WS      7 //8 
	#define I2S_PIN_DIN     6 //9 
	#define I2S_PIN_DOUT    10 //2

#elif CONFIG_DEVICE_3//
	#define I2S_PIN_BCLK    8 //7
	#define I2S_PIN_WS      7 //8 
	#define I2S_PIN_DIN     6 //9 
	#define I2S_PIN_DOUT    10 //6

#elif CONFIG_DEVICE_4//
	#define I2S_PIN_BCLK    8 //6
	#define I2S_PIN_WS      7 //5 
	#define I2S_PIN_DIN     6 //4 
	#define I2S_PIN_DOUT    10 //12

#endif


#endif // GPIO_CONFIG_H