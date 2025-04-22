#include <stdio.h>
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "esp_dsp.h"
#include "esp_system.h"
#include <math.h>
#include <malloc.h>
#include "dsp_common.h"
#include "esp_task_wdt.h"
#include <string.h>
#include "esp_heap_caps.h"
#include "fir_filter.h"
#include "gpio_config.h"
#include "webserver.h"
#include "esp_timer.h"
//#include "wm8805.h"

#define TAG "I2S_STD"

#define I2S_NUM         I2S_NUM_0
#define SAMPLE_RATE     44100

#define DMA_DESC_NUM 16
#define DMA_FRAME_NUM 256
#define GPIO_BUTTON_PIN 0      // GPIO пин за бутона
#define LONG_PRESS_TIME 2000   // Време за дълго натискане в милисекунди

 TaskHandle_t left_task_handle;
 TaskHandle_t right_task_handle;
 TaskHandle_t main_task_handle;

#ifdef CONFIG_ENABLE_ENCODER
#include "encoder.h"		
#else
const float volume_s = 1.0;	
#endif

#ifdef CONFIG_FILTER_FIR_IIR
#include "Biquad.h"
#elif CONFIG_FILTER_IIR
#include "Biquad.h"
#endif

#ifdef CONFIG_AUDIO_SOURCE_WM8805
#include "wm8805.h"
#endif

i2s_chan_handle_t rx_chan, tx_chan;

void init_i2s(void) {
	  // Конфигурация на I2S канала в SLAVE режим
    

    // Конфигурация за I2S канала
	 i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_SLAVE);
	 chan_cfg.auto_clear = true;
	 chan_cfg.auto_clear_before_cb = true;
	 chan_cfg.dma_desc_num = DMA_DESC_NUM;
	 chan_cfg.dma_frame_num = DMA_FRAME_NUM;
	  
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = -1,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = I2S_PIN_BCLK,
            .ws   = I2S_PIN_WS,
            .dout = I2S_PIN_DOUT,
            .din  = I2S_PIN_DIN, // In duplex mode, bind output and input to a same gpio can loopback internally
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
     /* Set data bit-width to 24 means your sample is 24 bits, I2S will help to shift the data by hardware */
    //std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_24BIT;
    /* Set slot bit-width to 32 means there still will be 32 bit in one slot */
   // std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    //std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT; //I2S_CLK_SRC_XTAL; //I2S_CLK_SRC_APLL;
    //std_cfg.clk_cfg.mclk_multiple = 384;
    //std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_EXTERNAL;  // Получава BCLK и WS от DIR9001
    //std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_128; // Не използва MCLK ext_clk_freq_hz  ext_clk_freq_hz 
    //std_cfg.clk_cfg.ext_clk_freq_hz = 22579200;
    //std_cfg .slot_cfg.left_align = true;
    /* Initialize the channels */
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

}
 void split_stereo_signal(int32_t *stereo_data, size_t num_samples, float *left, float *right) {
    int32_t *in = stereo_data;
    float *l = left, *r = right;
    const float scale = 1.0f / 8388608.0f; // 2^23

    for (size_t i = 0; i < num_samples; i++) {
        *l++ = (float)(*in++) * scale; // Ляв канал
        *r++ = (float)(*in++) * scale; // Десен канал
    }
}

void combine_stereo_signal(float *left, float *right, int32_t *stereo_data, size_t num_samples) {
    const float scale = 8388608.0f; // 2^23
    const float volume_gain = 1.3f;  // Например, усилване на 2 пъти

    float *l = left;
    float *r = right;
    int32_t *stereo = stereo_data;

    for (size_t i = 0; i < num_samples; i++) {
        #if CONFIG_FILTER_FIR
     //   *l *= volume_s;
     //   *r *= volume_s;
        #endif
        #if CONFIG_PHASE_INVERT
        *l *= -1;
        *r *= -1;
        #endif

     //   *l = fmaxf(-1.0f, fminf(*l, 1.0f));
     //   *r = fmaxf(-1.0f, fminf(*r, 1.0f));
		*l *= volume_gain;
		*r *= volume_gain;
        *stereo++ = (int32_t)(*l * scale);   // Ляв канал
        *stereo++ = (int32_t)(*r * scale);   // Десен канал

        l++;  // Преминаваме към следващия елемент в масива за левия канал
        r++;  // Преминаваме към следващия елемент в масива за десния канал
    }
}
/*
  void split_stereo_signal(const int32_t *stereo_data, size_t num_samples, float *left, float *right) {
    for (size_t i = 0; i < num_samples; i++) {
        left[i] = (float)stereo_data[i * 2] / 2147483648.0f;   // Ляв канал
        right[i] = (float)stereo_data[2 * i + 1] / 2147483648.0f; // Десен канал	
    } 
  }
 
  void combine_stereo_signal( float *left,  float *right, int32_t *stereo_data, size_t num_samples) {
    for (size_t i = 0; i < num_samples; i++) {
    	 #if CONFIG_FILTER_FIR
        left[i] *= volume_s;
 	    right[i] *= volume_s;
 	    #endif
 	    #if CONFIG_PHASE_INVERT
		left[i] *= -1;
 	    right[i] *= -1;
		#endif
 	    
 	   left[i] = fmaxf(-1.0f, fminf(left[i], 1.0f));
	   right[i] = fmaxf(-1.0f, fminf(right[i], 1.0f));
        stereo_data[2 * i] = (int32_t)(left[i] * 2147483648.0f);   // Ляв канал
        stereo_data[2 * i + 1] = (int32_t)(right[i] * 2147483648.0f); // Десен канал
    }
}

*/ 

#if CONFIG_PHASE_INVERT
		void process_phase(int32_t *data, size_t num_samples){
			for (size_t i = 0; i < num_samples; i++) {
				data[i] = data[i] * -1;
			}
		}
#endif
void print_selected_device(int val){
	ESP_LOGI("DEVICE", "Selected Device %d GPIO: WS %d BCLK %d, DIN %d DOUT %d",val, I2S_PIN_WS, I2S_PIN_BCLK, I2S_PIN_DIN, I2S_PIN_DOUT);
}
void print_device_select(){
    // Проверка на избраното устройство
    #if CONFIG_DEVICE_1
    	print_selected_device(1);
    	#if CONFIG_FILTER_FIR || CONFIG_FILTER_FIR_IIR
    	ESP_LOGW("FILTER", "Selected Phase Filter");
    	#endif

    #elif CONFIG_DEVICE_2
		print_selected_device(2);
		ESP_LOGW("FILTER", "Selected Low-Pass Filter");

    #elif CONFIG_DEVICE_3
        print_selected_device(3);
        ESP_LOGW("FILTER", "Selected Band-Pass Filter");

    #elif CONFIG_DEVICE_4
		print_selected_device(4);
		ESP_LOGW("FILTER", "Selected High-Pass Filter");

    #endif
}




static QueueHandle_t button_evt_queue = NULL;
static bool button_pressed = false;
static TickType_t press_start_time = 0;

// Структура за събития на бутона
typedef enum {
    BUTTON_PRESS_START,
    BUTTON_PRESS_END
} button_event_t;

// Функция за обработка на интерупт на бутона
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    button_event_t evt = gpio_get_level(GPIO_BUTTON_PIN) ? BUTTON_PRESS_END : BUTTON_PRESS_START;
    xQueueSendFromISR(button_evt_queue, &evt, NULL);
}

// Функция за обработка на събитията от бутона
void button_task(void* arg) {
    button_event_t evt;
    
    while (1) {
        if (xQueueReceive(button_evt_queue, &evt, portMAX_DELAY)) {
            if (evt == BUTTON_PRESS_START) {
                press_start_time = xTaskGetTickCount();  // Започваме да измерваме времето на натискане
                button_pressed = true;
            } else if (evt == BUTTON_PRESS_END && button_pressed) {
                TickType_t press_duration = xTaskGetTickCount() - press_start_time;
                if (press_duration >= pdMS_TO_TICKS(LONG_PRESS_TIME)) {
                    // Ако бутонът е натиснат за повече от 5 секунди
                    ESP_LOGI("BUTTON", "Button pressed for more than 5 seconds");
                   ESP_LOGI(TAG, "Стартиране на Wi-Fi AP...");
   					wifi_init_softap();
   					vTaskDelay(pdMS_TO_TICKS(100));
   					ESP_LOGI(TAG, "Стартиране на уеб сървъра...");
   					start_webserver();
   					vTaskSuspend(main_task_handle);
   					vTaskSuspend(left_task_handle);
   					vTaskSuspend(right_task_handle);
   					vTaskDelay(pdMS_TO_TICKS(200)); 
					
					    ESP_ERROR_CHECK(i2s_channel_disable(rx_chan));
					    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
					    
					    ESP_ERROR_CHECK(i2s_del_channel(rx_chan));
					    ESP_ERROR_CHECK(i2s_del_channel(tx_chan));
					    vTaskDelay(pdMS_TO_TICKS(10));
					ESP_LOGW("I2S", "Delete i2s channel");
                } else {
                    ESP_LOGI("BUTTON", "Button press was too short");
                }
                button_pressed = false;  // Ресет на натискането
            }
        }
    }
}

// Инициализация на GPIO и интерупта
void gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE  // Интерупт при всяка промяна на нивото
    };
    gpio_config(&io_conf);

    // Създаване на опашка за събития от бутона
    button_evt_queue = xQueueCreate(10, sizeof(button_event_t));

    // Настройка на интерупт
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(GPIO_BUTTON_PIN, gpio_isr_handler, NULL);

    // Създаване на задача за обработка на събития от бутона
    xTaskCreate(button_task, "button_task", 4096, NULL, 2, NULL);
}

void app_main(void) {

    esp_err_t err = web_server();
     if(err != ESP_OK){
     return;
     }
    gpio_init();
	init_i2s();
	

	#if CONFIG_AUDIO_SOURCE_WM8805
	i2c_wm8805_init();
	#endif

    // Проверка на избрания филтър
    #if CONFIG_FILTER_NONE
    	ESP_LOGI(TAG,"No filter applied");
    	ESP_LOGW(TAG,"Passthrough Mode");
    #elif CONFIG_FILTER_FIR || CONFIG_FILTER_FIR_IIR
    	fir_filter_init();
		//int32_t *data_fir = (int32_t *)malloc(BUFFER_SIZE * sizeof(int32_t));
		int32_t *data_fir = (int32_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(int32_t), MALLOC_CAP_DMA);
		if (data_fir == NULL) {
   		 	ESP_LOGE(TAG, "Failed to allocate FIR buffer!");
    		return;
		}
		    // Създаване на задачи за обработка
    	xTaskCreatePinnedToCore(process_left, "Left Task", 4096, NULL, 5, &left_task_handle, 0);
    	xTaskCreatePinnedToCore(process_right, "Right Task", 4096, NULL, 5, &right_task_handle, 1);
   		#if CONFIG_FILTER_FIR_IIR
   		create_biquad();
    	ESP_LOGI(TAG,"FIR and IIR filter selected");
    	#elif CONFIG_FILTER_FIR
    	ESP_LOGI(TAG,"FIR filter selected");
    	#endif
    #elif CONFIG_FILTER_IIR
    	create_biquad();
        ESP_LOGI(TAG,"IIR filter selected");
        // Включи IIR код тук
    #endif

print_device_select();

#if CONFIG_ENABLE_ENCODER
	 // Инициализация на енкодер
    init_encoder_interrupts();
    // Създаване на задача за енкодера
    xTaskCreatePinnedToCore(encoder_task, "Encoder Task", 4096, NULL, 3, NULL, 1);
#endif
	//TODO
	//init_encoder();	
	
	main_task_handle = xTaskGetCurrentTaskHandle();
    
   // int32_t *data = (int32_t *)malloc(BUFFER_SIZE * sizeof(int32_t));
    int32_t *data = (int32_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(int32_t), MALLOC_CAP_DMA);
	if (data == NULL) {
   	 	ESP_LOGE(TAG, "Failed to allocate Data buffer!");
    	return;
	}
 	
    size_t bytes_read;
  //  size_t bytes_written;
    // Основен цикъл
    ESP_LOGW(TAG, "Start AUDIO");
    ESP_LOGI("MEMORY", "Free internal SRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
while (1) {
        // Четене на данни от I2S (DIR9001)
        esp_err_t err = i2s_channel_read(rx_chan, data, BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    	if (err != ESP_OK){
    		ESP_LOGE(TAG, "STOP I2S");
    		return;
    	}
#if CONFIG_FILTER_NONE
       #if CONFIG_PHASE_INVERT
		  process_phase(data, BUFFER_SIZE);
		#endif
		i2s_channel_write(tx_chan, data, BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);
#elif CONFIG_FILTER_FIR
       split_stereo_signal(data, BUFFER_SIZE/2 , fir_input_left, fir_input_right);
 		
        // Изпращане на сигнал към задачите за обработка
        xTaskNotifyGive(left_task_handle);
        xTaskNotifyGive(right_task_handle);

        // Изчакване задачите да приключат
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

 	    combine_stereo_signal(fir_output_left, fir_output_right, data_fir, BUFFER_SIZE/2 );
 	    i2s_channel_write(tx_chan, data_fir, BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);
        
        // Включи FIR код тук
#elif CONFIG_FILTER_IIR
        // Използвай IIR филтър
		process_data_stereo(data, bytes_read / sizeof(int32_t), volume_s);
		#if CONFIG_PHASE_INVERT
		  process_phase(data, BUFFER_SIZE);
		#endif
		i2s_channel_write(tx_chan, data, BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);
        
    #elif CONFIG_FILTER_FIR_IIR
        // Включи комбинация от FIR и IIR код тук
         process_data_stereo(data, bytes_read / sizeof(int32_t), volume_s);

         split_stereo_signal(data, BUFFER_SIZE/2 , fir_input_left, fir_input_right);
 		
        // Изпращане на сигнал към задачите за обработка
        xTaskNotifyGive(left_task_handle);
        xTaskNotifyGive(right_task_handle);

        // Изчакване задачите да приключат
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

 	    combine_stereo_signal(fir_output_left, fir_output_right, data_fir, BUFFER_SIZE/2 );
 	    i2s_channel_write(tx_chan, data_fir, BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);
        
    #endif    
       /* 
		memset(data, 0, sizeof(data));
		memset(data_fir, 0, sizeof(data_fir));
		memset(fir_input_left, 0, sizeof(fir_input_left));
		memset(fir_input_right, 0, sizeof(fir_input_right));
		memset(fir_output_left, 0, sizeof(fir_output_left));
		memset(fir_output_right, 0, sizeof(fir_output_right));
		*/

    }
    

}
