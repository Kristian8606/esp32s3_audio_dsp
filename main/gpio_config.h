#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

    // Проверка на избраното устройство
#if CONFIG_DEVICE_1
    // Дефиниране на GPIO пинове
	#define I2S_PIN_BCLK     6  // BCLK
	#define I2S_PIN_WS       5  // WS
	#define I2S_PIN_DIN      10  // DIN вход
	#define I2S_PIN_DOUT     4  // DOUT изход
	
#elif CONFIG_DEVICE_2
	#define I2S_PIN_BCLK     7
	#define I2S_PIN_WS       8 
	#define I2S_PIN_DIN      11 
	#define I2S_PIN_DOUT     2

#elif CONFIG_DEVICE_3
	#define I2S_PIN_BCLK     7
	#define I2S_PIN_WS       8 
	#define I2S_PIN_DIN      9 
	#define I2S_PIN_DOUT     6

#elif CONFIG_DEVICE_4
	#define I2S_PIN_BCLK     6
	#define I2S_PIN_WS       5 
	#define I2S_PIN_DIN      2 
	#define I2S_PIN_DOUT     12

#endif


#endif // GPIO_CONFIG_H