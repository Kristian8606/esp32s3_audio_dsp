#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_system.h"

#define MAX_COEFFS 1024
#define MAX_POST_SIZE 25000  // 25 KB

// Прототипи на функциите
esp_err_t save_coefficients(void);
esp_err_t load_coefficients(void);
esp_err_t get_handler(httpd_req_t *req);
esp_err_t post_handler(httpd_req_t *req);
httpd_handle_t start_webserver(void);
void wifi_init_softap(void);
esp_err_t web_server(void);

//static __attribute__((aligned(16))) double fir_coeffs[MAX_COEFFS] = {0};
extern float coeffs[MAX_COEFFS];  // Масив за коефициентите
extern int32_t coeff_count;        // Брой на коефициентите
extern char buf[MAX_POST_SIZE];    // Буфер за заявката
extern char json_buffer[MAX_POST_SIZE]; // Буфер за JSON данните

extern int32_t filter_count; 
extern int type_filters[];
extern double Hz[];
extern double dB[];
extern double Qd[];

#endif // WEB_SERVER_H
