#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "esp_system.h"
#include "webserver.h"
#include "fir_coeffs.h"


#define MAX_COEFFS 1024
#define MAX_POST_SIZE 25000  // 25 KB

static const char *TAG = "WEB_SERVER";
 float coeffs[MAX_COEFFS];
 int32_t coeff_count = 0;
char buf[MAX_POST_SIZE];
char json_buffer[MAX_POST_SIZE];

// Запазване на коефициентите в NVS
esp_err_t save_coefficients() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при отваряне на NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_i32(nvs_handle, "coeff_count", coeff_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при запис на coeff_count в NVS!");
    } else {
        err = nvs_set_blob(nvs_handle, "coeffs", coeffs, coeff_count * sizeof(float));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Грешка при запис на коефициентите в NVS!");
        } else {
            ESP_LOGI(TAG, "Запазени %d коефициента.", coeff_count);
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
            return err;
        }
    }
    nvs_close(nvs_handle);
    return err;
}

// Зареждане на коефициентите от NVS
esp_err_t load_coefficients() {
    nvs_handle_t nvs_handle;
    size_t required_size = 0;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при отваряне на NVS: %s", esp_err_to_name(err));
        return err;
    }

    if (nvs_get_i32(nvs_handle, "coeff_count", &coeff_count) != ESP_OK || coeff_count > MAX_COEFFS) {
        ESP_LOGW(TAG, "Няма записани коефициенти или надвишават лимита.");
        coeff_count = 0;
    }

    err = nvs_get_blob(nvs_handle, "coeffs", NULL, &required_size);
    if (err == ESP_OK && required_size <= sizeof(coeffs)) {
        err = nvs_get_blob(nvs_handle, "coeffs", coeffs, &required_size);
        if (err == ESP_OK) {
            ESP_LOGW(TAG, "Заредени %d коефициента:", coeff_count);
    /*		for (int i = 0; i < coeff_count; i++) {
   				 if (i % 4 == 0) {
   				    printf("\n");
   				 }
   				// fir_coeffs[i]= (float)coeffs[i];
   				 printf("%.20f ", (float)coeffs[i]);
			}
			printf("\n");
			*/
        } else {
            ESP_LOGE(TAG, "Грешка при четене на коефициентите: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "Няма наличен запис за коефициентите или размерът не е коректен.");
    }

    nvs_close(nvs_handle);
    
     if(coeff_count < 40){
    	return ESP_FAIL;
    }
    return ESP_OK;
}


// HTML страница за въвеждане на коефициенти
const char *html_page = "<html><head>"
								"<style>"
								"  body { display: flex; justify-content: center; align-items: center; height: 100vh; flex-direction: column; font-family: Arial, sans-serif; }"
								"  h2 { text-align: center; }"
								"  textarea { width: 60%; height: 300px; font-size: 16px; padding: 10px; }"
								"  button { margin-top: 10px; padding: 10px 20px; font-size: 16px; cursor: pointer; background: #007bff; color: white; border: none; border-radius: 5px; }"
								"  button:hover { background: #0056b3; }"
								"</style>"
								"</head><body>"
								"<h2>Set Coefficients</h2>"
								"<textarea id='coeffs' placeholder='Enter coefficients separated by space, comma, or new line...'></textarea><br>"
								"<button onclick='sendData()'>Submit</button>"
								"<script>"
								"function sendData() {"
								"  let rawData = document.getElementById('coeffs').value.split(/[ ,\\n]+/).map(Number);"
								"  let validData = rawData.filter(n => !isNaN(n));"
								"  fetch('/set', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({ coeffs: validData }) })"
								"  .then(res => res.text()).then(alert);"
								"}"
								"</script></body></html>";

// HTTP GET заявка (HTML)
esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP POST заявка (JSON)
esp_err_t post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Получена POST заявка с дължина %d байта", req->content_len);
    
    int total_len = req->content_len;
    int received = 0;

    // Проверка за размер на заявката
    if (total_len > MAX_POST_SIZE) {
        ESP_LOGE(TAG, "Прекалено голяма заявка! Размер: %d байта", total_len);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Заявката е твърде голяма");
        return ESP_FAIL;
    }

    // Четене на POST данните
    while (received < total_len) {
        int ret = httpd_req_recv(req, buf + received, total_len - received);
        if (ret <= 0) {
            ESP_LOGE(TAG, "Грешка при получаване на данни: %d", ret);
            return ESP_FAIL;
        }
        received += ret;
    }

    buf[received] = '\0';  // Гарантиране на валиден JSON стринг
    ESP_LOGI(TAG, "Получени сурови JSON данни: %s", buf);

    // Записваме JSON в глобален буфер
    strncpy(json_buffer, buf, sizeof(json_buffer) - 1);

    // Парсиране на JSON
    cJSON *json = cJSON_Parse(json_buffer);
    if (!json) {
        ESP_LOGE(TAG, "Грешка при парсинг на JSON! Съдържание: %s", json_buffer);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Невалиден JSON формат");
        return ESP_FAIL;
    }

    // Извличане на "coeffs" масива
    cJSON *coeff_array = cJSON_GetObjectItem(json, "coeffs");
    if (!coeff_array || !cJSON_IsArray(coeff_array)) {
        ESP_LOGE(TAG, "Невалиден JSON: липсва масив coeffs!");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Очаква се масив 'coeffs'");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    int num_coeffs = cJSON_GetArraySize(coeff_array);
    ESP_LOGI(TAG, "Получени %d коефициента", num_coeffs);

    if (num_coeffs > MAX_COEFFS) {
        ESP_LOGW(TAG, "Прекалено много коефициенти, ограничаваме до %d!", MAX_COEFFS);
        num_coeffs = MAX_COEFFS;
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Elements is > 1024");
    }

    // Запис на коефициентите
    for (int i = 0; i < num_coeffs; i++) {
        cJSON *coeff_item = cJSON_GetArrayItem(coeff_array, i);
        if (coeff_item && cJSON_IsNumber(coeff_item)) {
            coeffs[i] = (float)coeff_item->valuedouble;
        } else {
            ESP_LOGW(TAG, "Некоректен коефициент на позиция %d", i);
        }
    }

    coeff_count = num_coeffs;
    cJSON_Delete(json);
     esp_err_t err = save_coefficients(); // Запис и рестарт
    if (err != ESP_OK){
    	return err;
    }
    
    httpd_resp_send(req, "The coefficients have been saved, ESP will restart!", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();  // Рестарт след запис
    return ESP_OK;
}


// Стартиране на уеб сървъра
httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/", .method = HTTP_GET, .handler = get_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/set", .method = HTTP_POST, .handler = post_handler });
    }
    return server;
}

// Стартиране на Wi-Fi AP
void wifi_init_softap() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t wifi_config = { .ap = { .ssid = "ESP32_Config", .password = "12345678", .max_connection = 4, .authmode = WIFI_AUTH_WPA_WPA2_PSK } };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}

esp_err_t web_server() {
    ESP_LOGI(TAG, "Инициализация на NVS...");
    esp_err_t errnvs = nvs_flash_init();
    if(errnvs == ESP_OK) ESP_LOGW(TAG, "NVS OK...");
    
    
   esp_err_t err = load_coefficients();
   
   if(err != ESP_OK){
    ESP_LOGI(TAG, "Стартиране на Wi-Fi AP...");
    wifi_init_softap();
    ESP_LOGI(TAG, "Стартиране на уеб сървъра...");
    start_webserver();
    
        ESP_LOGI("MEMORY", "Free internal SRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
	}else{
		ESP_LOGI("MEMORY", "Free internal SRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
		ESP_LOGW(TAG, "ESP load coeff done");
		return ESP_OK;
	}
	return err;
}