#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "esp_system.h"
#include "webserver.h"
#include "fir_coeffs.h"
#include "esp_tls.h"
#include "esp_netif.h"


#define MAX_COEFFS 1024
#define MAX_POST_SIZE 25000  // 25 KB

static const char *TAG = "WEB_SERVER";
 float coeffs[MAX_COEFFS];
 int32_t coeff_count = 0;
char buf[MAX_POST_SIZE];
char json_buffer[MAX_POST_SIZE];

#define MAX_FILTERS 10  // Максимален брой филтри
int32_t filter_count ; 
int type_filters[MAX_FILTERS];
double Hz[MAX_FILTERS];
double dB[MAX_FILTERS];
double Qd[MAX_FILTERS];





// HTML страница за въвеждане на коефициенти
#if CONFIG_DEVICE_1
esp_err_t save_filters() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при отваряне на NVS: %s", esp_err_to_name(err));
        return err;
    }

    if (filter_count <= 0 || filter_count > MAX_FILTERS) {
        ESP_LOGE(TAG, "Некоректен filter_count: %d" , filter_count);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Записваме %d филтъра...", filter_count);

    err = nvs_set_i32(nvs_handle, "filter_count", filter_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при запис на filter_count: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Запис на филтрите
    err = nvs_set_blob(nvs_handle, "type_filters", type_filters, filter_count * sizeof(int));
    err |= nvs_set_blob(nvs_handle, "Hz", Hz, filter_count * sizeof(double));
    err |= nvs_set_blob(nvs_handle, "dB", dB, filter_count * sizeof(double));
    err |= nvs_set_blob(nvs_handle, "Qd", Qd, filter_count * sizeof(double));

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при запис на филтрите в NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Записани филтри: %d", filter_count);
        for(int i = 0; i< filter_count; i++){
        ESP_LOGW(TAG, "Филтър: Type: %d, Freq: %.2f Hz, Gain: %.2f dB, Q: %.2f", 
                 type_filters[i], Hz[i], dB[i], Qd[i]);
		}
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Грешка при потвърждаване на записите в NVS: %s", esp_err_to_name(err));
        }
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_filters() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при отваряне на NVS за четене: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_i32(nvs_handle, "filter_count", &filter_count);
    if (err != ESP_OK || filter_count <= 0) {
        ESP_LOGE(TAG, "Грешка при четене на filter_count! Стойност: %d", filter_count);
        filter_count = 0; // Предпазна мярка
    } else {
        ESP_LOGI(TAG, "Зареждаме %d филтъра...", filter_count);

        size_t size = filter_count * sizeof(int);
        err = nvs_get_blob(nvs_handle, "type_filters", type_filters, &size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Грешка при зареждане на type_filters: %s", esp_err_to_name(err));
        }

        size = filter_count * sizeof(double);
        err |= nvs_get_blob(nvs_handle, "Hz", Hz, &size);
        err |= nvs_get_blob(nvs_handle, "dB", dB, &size);
        err |= nvs_get_blob(nvs_handle, "Qd", Qd, &size);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Грешка при зареждане на филтрите в NVS: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Заредени филтри: %d", filter_count);
           for(int i = 0; i< filter_count; i++){
        ESP_LOGW(TAG, "Филтър: Type: %d, Freq: %.2f Hz, Gain: %.2f dB, Q: %.2f", 
                 type_filters[i], Hz[i], dB[i], Qd[i]);
		}
        }
    }

    nvs_close(nvs_handle);
    return err;
}
          
          
          
const char *html_page = 
    "<html><head><title>Filter Config</title>"
    "<style>"
    "  body { font-family: Arial, sans-serif; display: flex; flex-direction: column; align-items: center; margin-top: 20px; }"
    "  .filter-container { width: 90%; display: flex; flex-direction: column; gap: 10px; }"
    "  .filter { display: flex; align-items: center; gap: 10px; border: 1px solid #ccc; padding: 10px; border-radius: 5px; }"
    "  select, input { padding: 8px; font-size: 14px; }"
    "  button { padding: 10px 20px; font-size: 16px; cursor: pointer; background: #007bff; color: white; border: none; border-radius: 5px; }"
    "  button:hover { background: #0056b3; }"
    "  .remove-btn { background: #ff4d4d; }"
    "  .remove-btn:hover { background: #cc0000; }"
    "</style></head><body>"

    "<h2>Configure Filters</h2>"
    "<div id='filters' class='filter-container'>"
    "  <!-- Filters will be loaded dynamically -->"
    "</div>"

    "<button onclick='addFilter()'>Add Filter</button>"
    "<button onclick='sendData()'>Submit</button>"

    "<script>"
    "function addFilter() {"
    "  let container = document.getElementById('filters');"
    "  let newFilter = document.createElement('div');"
    "  newFilter.classList.add('filter');"
    "  newFilter.innerHTML = \""
    "    <select name='type'>"
    "      <option value='4'>Peak</option>"
    "      <option value='5'>Low Shelf</option>"
    "      <option value='6'>High Shelf</option>"
    "    </select>"
    "    <input type='number' name='freq' placeholder='Frequency (Hz)'>"
    "    <input type='number' name='gain' placeholder='Gain (dB)'>"
    "    <input type='number' name='q' placeholder='Q Factor'>"
    "    <button class='remove-btn' onclick='removeFilter(this)'>Remove</button>"
    "  \";"
    "  container.appendChild(newFilter);"
    "}"

    "function removeFilter(btn) {"
    "  btn.parentNode.remove();"
    "}"

    "function sendData() {"
    "  let filters = [];"
    "  document.querySelectorAll('.filter').forEach(div => {"
    "    let type = parseInt(div.querySelector('[name=\\\"type\\\"]').value);"
    "    let freq = parseFloat(div.querySelector('[name=\\\"freq\\\"]').value);"
    "    let gain = parseFloat(div.querySelector('[name=\\\"gain\\\"]').value);"
    "    let q = parseFloat(div.querySelector('[name=\\\"q\\\"]').value);"
    "    filters.push({ type, freq, gain, q });"
    "  });"
    "  fetch('/set', {"
    "    method: 'POST',"
    "    headers: { 'Content-Type': 'application/json' },"
    "    body: JSON.stringify({ filters })"
    "  }).then(res => res.text()).then(alert);"
    "}"

    // Loading filters from the server
    "window.onload = function() {"
    "  fetch('/getFilters')"
    "  .then(response => response.json())"
    "  .then(filters => {"
    "    let container = document.getElementById('filters');"
    "    filters.forEach(filter => {"
    "      let newFilter = document.createElement('div');"
    "      newFilter.classList.add('filter');"
    "      newFilter.innerHTML = `"
    "        <select name='type'>"
    "          <option value='4' ${filter.type === 4 ? 'selected' : ''}>Peak</option>"
    "          <option value='5' ${filter.type === 5 ? 'selected' : ''}>Low Shelf</option>"
    "          <option value='6' ${filter.type === 6 ? 'selected' : ''}>High Shelf</option>"
    "        </select>"
    "        <input type='number' name='freq' value='${filter.freq}' placeholder='Frequency (Hz)'>"
    "        <input type='number' name='gain' value='${filter.gain}' placeholder='Gain (dB)'>"
    "        <input type='number' name='q' value='${filter.q}' placeholder='Q Factor'>"
    "        <button class='remove-btn' onclick='removeFilter(this)'>Remove</button>"
    "      `;"
    "      container.appendChild(newFilter);"
    "    });"
    "  })"
    "  .catch(error => console.error('Error loading filters:', error));"
    "};"
    "</script></body></html>";

#else
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
#endif
								

// HTTP GET заявка (HTML)
esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



#if CONFIG_DEVICE_1



typedef struct {
    int type;   // Тип на филтъра (peak, lowshelf, highshelf)
    float freq;      // Честота в Hz
    float gain;      // Усилване в dB
    float q;         // Q фактор
} Filter;

Filter filters[MAX_FILTERS];  // Масив от филтри
        // Брой добавени филтри

esp_err_t set_filter_handler(httpd_req_t *req) {
    char content[1024];  
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
        return ESP_FAIL;
    }
    content[ret] = '\0';  // Добавяме null terminator

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *filters_json = cJSON_GetObjectItem(json, "filters");
    if (!cJSON_IsArray(filters_json)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }

    filter_count = 0;
    int array_size = cJSON_GetArraySize(filters_json);
    for (int i = 0; i < array_size && i < MAX_FILTERS; i++) {
        cJSON *filter_json = cJSON_GetArrayItem(filters_json, i);
        if (!cJSON_IsObject(filter_json)) continue;

        cJSON *type = cJSON_GetObjectItem(filter_json, "type");
        cJSON *freq = cJSON_GetObjectItem(filter_json, "freq");
        cJSON *gain = cJSON_GetObjectItem(filter_json, "gain");
        cJSON *q = cJSON_GetObjectItem(filter_json, "q");

        if (cJSON_IsNumber(type) && cJSON_IsNumber(freq) && cJSON_IsNumber(gain) && cJSON_IsNumber(q)) {
           // strncpy(filters[filter_count].type, type->valuestring, sizeof(filters[filter_count].type) - 1);
           filters[filter_count].type = type->valueint;
            filters[filter_count].freq = freq->valuedouble;
            filters[filter_count].gain = gain->valuedouble;
            filters[filter_count].q = q->valuedouble;
            filter_count++;
        }
    }
    
	printf("Received Filters: %ld\n", filter_count);
    for (int i = 0; i < filter_count; i++) {
        printf("Filter %d: Type: %d, Freq: %.2f Hz, Gain: %.2f dB, Q: %.2f\n",
               i + 1, filters[i].type, filters[i].freq, filters[i].gain, filters[i].q);
         type_filters[i] = filters[i].type;
		 Hz[i] = filters[i].freq;
		 dB[i] = filters[i].gain;
		 Qd[i] = filters[i].q;
		
    }
    cJSON_Delete(json);
    esp_err_t err = save_filters();
    if(err == ESP_OK){
     httpd_resp_send(req, "The filters have been saved, ESP will restart!", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();  // Рестарт след запис
    }
   // create_biquad();
    return ESP_OK;
}
#else
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
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();  // Рестарт след запис
    return ESP_OK;
}
#endif

esp_err_t get_filters_handler(httpd_req_t *req) {
    // Проверка дали има филтри
    if (filter_count == 0) {
        httpd_resp_send(req, "No filters available", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    // Създаване на JSON обект за филтри
    cJSON *filters_array = cJSON_CreateArray();

    for (int i = 0; i < filter_count; i++) {
        cJSON *filter = cJSON_CreateObject();
        cJSON_AddNumberToObject(filter, "type", type_filters[i]);
        cJSON_AddNumberToObject(filter, "freq", Hz[i]);
        cJSON_AddNumberToObject(filter, "gain", dB[i]);
        cJSON_AddNumberToObject(filter, "q", Qd[i]);
        cJSON_AddItemToArray(filters_array, filter);
    }

    // Преобразуване на JSON в string
    char *response = cJSON_Print(filters_array);
    // Изпращане на отговор към клиента
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);

    // Освобождаване на паметта
    free(response);
    cJSON_Delete(filters_array);
    return ESP_OK;
}
// Стартиране на уеб сървъра
httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    
  


    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/", .method = HTTP_GET, .handler = get_handler });
        
        #if CONFIG_DEVICE_1
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/getFilters", .method = HTTP_GET, .handler = get_filters_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/set", .method = HTTP_POST, .handler = set_filter_handler });
        
        #else
         httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/set", .method = HTTP_POST, .handler = post_handler });
        #endif
      //  httpd_register_uri_handler(server, &set_filter_uri);
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
    
    #if CONFIG_DEVICE_1
    esp_err_t err = load_filters();
    #else
    esp_err_t err = load_coefficients();
    #endif
   
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