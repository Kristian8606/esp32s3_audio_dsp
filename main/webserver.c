

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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_COEFFS 1024
#define MAX_POST_SIZE 25000  // 25 KB
#define MAX_FILTERS 20       // Максимален брой филтри

static const char *TAG = "WEB_SERVER";

/* --- Coeffs (DSP) --- */

/* --- Filters (EQ) --- */

float coeffs[MAX_COEFFS];
int32_t coeff_count = 0;
int32_t filter_count = 0;

int type_filters[MAX_FILTERS];
double Hz[MAX_FILTERS];
double dB[MAX_FILTERS];
double Qd[MAX_FILTERS];

char buf[MAX_POST_SIZE];
char json_buffer[MAX_POST_SIZE];

/* Допълнителна структура, за удобство */
typedef struct {
    int type;
    float freq;
    float gain;
    float q;
} Filter;
static Filter filters[MAX_FILTERS];

/* --- Общи буфери --- */
 char buf[MAX_POST_SIZE];
 char json_buffer[MAX_POST_SIZE];

 bool is_eq = false;
bool bypass_state = false;
bool phase_state = false;

#define DEVICE_KEY "is_eq"

/* --- NVS: device type (EQ / DSP) --- */
esp_err_t save_device_type(bool val) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_u8(h, DEVICE_KEY, val ? 1 : 0);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t load_device_type(bool *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    uint8_t v = 0;
    err = nvs_get_u8(h, DEVICE_KEY, &v);
    nvs_close(h);
    *out = v ? true : false;
    return err;
}

/* --- HTML pages --- */
const char *html_landing_page =
"<html><head><title>Select Device</title>"
"<style>"
"body { display:flex; flex-direction:column; justify-content:center; align-items:center; height:100vh; font-family:Arial,sans-serif; }"
"button { padding:15px 30px; font-size:18px; margin:10px; cursor:pointer; border:none; border-radius:5px; color:white; }"
".eq { background:#28a745; } .dsp { background:#007bff; }"
".eq:hover { background:#1e7e34; } .dsp:hover { background:#0056b3; }"
"</style></head><body>"
"<h2>Select Device Type</h2>"
"<button class='eq' onclick='selectDevice(true)'>EQ</button>"
"<button class='dsp' onclick='selectDevice(false)'>FIR</button>"
"<script>"
"function selectDevice(isEQ) {"
"  fetch('/setDevice', { method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({is_eq:isEQ}) })"
"  .then(()=>{ location.reload(); });"
"}"
"</script>"
"</body></html>";

const char *html_page_dsp =
"<html><head>"
"<style>"
"body { display: flex; justify-content: center; align-items: center; height: 100vh; flex-direction: column; font-family: Arial, sans-serif; }"
"textarea { width: 60%; height: 300px; font-size: 16px; padding: 10px; }"
"button { margin-top: 10px; padding: 10px 20px; font-size: 16px; cursor: pointer; background: #007bff; color: white; border: none; border-radius: 5px; }"
"button:hover { background: #0056b3; }"
"</style></head><body>"
"<h2>Set Coefficients (DSP)</h2>"
"<textarea id='coeffs' placeholder='Enter coefficients separated by space, comma, or new line...'></textarea><br>"
"<button onclick='sendData()'>Submit</button>"
"<script>"
"async function sendData() {"
"  let rawData = document.getElementById('coeffs').value.split(/[ ,\\n]+/).map(Number);"
"  let validData = rawData.filter(n => !isNaN(n));"
"  const res = await fetch('/setCoeffs', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({ coeffs: validData }) });"
"  alert(await res.text());"
"  setTimeout(()=>location.reload(), 300);"
"}"
"// optionally load existing coeffs to textarea"
"window.onload = async function(){"
"  try {"
"    const r = await fetch('/getCoeffs');"
"    if (r.ok) {"
"      const j = await r.json();"
"      if (j.coeffs && Array.isArray(j.coeffs) && j.coeffs.length>0) {"
"        document.getElementById('coeffs').value = j.coeffs.join('\\n');"
"      }"
"    }"
"  } catch(e){}"
"};"
"</script></body></html>";

const char *html_page_eq =
"<html><head><title>Filter Config</title>"
"<style>"
"body { font-family: Arial, sans-serif; display: flex; flex-direction: column; align-items: center; margin-top: 20px; background: #f4f4f9; }"
"h2 { color: #333; margin-bottom: 20px; }"
".filter-container { width: 90%; display: flex; flex-direction: column; gap: 10px; margin-bottom: 20px; }"
".filter { display: flex; align-items: center; gap: 10px; border: 1px solid #ccc; padding: 10px; border-radius: 8px; background: #fff; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }"
"select, input { padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 5px; }"
"button { padding: 10px 25px; font-size: 16px; cursor: pointer; border: none; border-radius: 5px; transition: 0.3s; box-shadow: 0 2px 5px rgba(0,0,0,0.2); }"
"button:hover { transform: translateY(-2px); box-shadow: 0 4px 10px rgba(0,0,0,0.3); }"
".add-btn { background: #28a745; color: white; margin-right: 10px; } .add-btn:hover { background: #218838; }"
".submit-btn { background: #007bff; color: white; } .submit-btn:hover { background: #0056b3; }"
".remove-btn { background: #dc3545; color: white; } .remove-btn:hover { background: #c82333; }"
".options { margin: 15px 0; display: flex; gap: 20px; font-size: 16px; }"
"</style></head><body>"
"<h2>Configure Filters</h2>"

"<div class='options'>"
"<label><input type='checkbox' id='bypass'> Bypass</label>"
"<label><input type='checkbox' id='phase'> Phase Invert</label>"
"</div>"

"<div id='filters' class='filter-container'></div>"
"<div style='margin-bottom:20px;'><button class='add-btn' onclick='addFilter()'>Add Filter</button>"
"<button class='submit-btn' onclick='sendData()'>Submit</button></div>"

"<script>"
"function addFilter(typeVal,freqVal,gainVal,qVal){"
" let container=document.getElementById('filters');"
" let newFilter=document.createElement('div');newFilter.classList.add('filter');"
" newFilter.innerHTML=\"<select name='type'><option value='4'>Peak</option><option value='5'>Low Shelf</option><option value='6'>High Shelf</option></select>\"+"
" \"<input type='number' name='freq' placeholder='Frequency (Hz)'>\"+"
" \"<input type='number' name='gain' placeholder='Gain (dB)'>\"+"
" \"<input type='number' name='q' placeholder='Q Factor'>\"+"
" \"<button class='remove-btn' onclick='removeFilter(this)'>Remove</button>\";"
" container.appendChild(newFilter);"
" if(typeVal!==undefined)newFilter.querySelector('[name=\"type\"]').value=typeVal;"
" if(freqVal!==undefined)newFilter.querySelector('[name=\"freq\"]').value=freqVal;"
" if(gainVal!==undefined)newFilter.querySelector('[name=\"gain\"]').value=gainVal;"
" if(qVal!==undefined)newFilter.querySelector('[name=\"q\"]').value=qVal;"
"}"

"function removeFilter(btn){btn.parentNode.remove();}"

"function sendData(){"
" let filters=[];"
" document.querySelectorAll('.filter').forEach(div=>{"
"   let type=parseInt(div.querySelector('[name=\"type\"]').value);"
"   let freq=parseFloat(div.querySelector('[name=\"freq\"]').value);"
"   let gain=parseFloat(div.querySelector('[name=\"gain\"]').value);"
"   let q=parseFloat(div.querySelector('[name=\"q\"]').value);"
"   filters.push({type,freq,gain,q});"
" });"
" let bypass=document.getElementById('bypass').checked;"
" let phase=document.getElementById('phase').checked;"
" fetch('/setFilters',{method:'POST',headers:{'Content-Type':'application/json'},"
" body:JSON.stringify({filters,bypass,phase})})"
" .then(res=>res.text()).then(alert);"
"}"

"window.onload=function(){"
" fetch('/getFilters').then(r=>r.json()).then(data=>{"
"   if(Array.isArray(data.filters)&&data.filters.length>0){"
"     data.filters.forEach(f=>addFilter(f.type,f.freq,f.gain,f.q));"
"   }"
"   if(data.bypass!==undefined)document.getElementById('bypass').checked=data.bypass;"
"   if(data.phase!==undefined)document.getElementById('phase').checked=data.phase;"
" }).catch(e=>console.error(e));"
"}"
"</script></body></html>";



/* --- Save / Load Filters --- */
esp_err_t save_filters() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при отваряне на NVS: %s", esp_err_to_name(err));
        return err;
    }

    if (filter_count < 0 || filter_count > MAX_FILTERS) {
        ESP_LOGE(TAG, "Некоректен filter_count: %d", filter_count);
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

    err = nvs_set_blob(nvs_handle, "type_filters", type_filters, filter_count * sizeof(int));
    err |= nvs_set_blob(nvs_handle, "Hz", Hz, filter_count * sizeof(double));
    err |= nvs_set_blob(nvs_handle, "dB", dB, filter_count * sizeof(double));
    err |= nvs_set_blob(nvs_handle, "Qd", Qd, filter_count * sizeof(double));
    err |= nvs_set_u8(nvs_handle, "bypass_state", bypass_state ? 1 : 0);
    err |= nvs_set_u8(nvs_handle, "phase_state", phase_state ? 1 : 0);


    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при запис на филтрите в NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Записани филтри: %d", filter_count);
        for (int i = 0; i < filter_count; i++) {
            ESP_LOGW(TAG, "Филтър: Type: %d, Freq: %.2f Hz, Gain: %.2f dB, Q: %.2f",
                     type_filters[i], Hz[i], dB[i], Qd[i]);
        }
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Грешка при commit: %s", esp_err_to_name(err));
        }
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_filters() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open(read) failed for filters: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_i32(nvs_handle, "filter_count", &filter_count);
    if (err != ESP_OK || filter_count <= 0 || filter_count > MAX_FILTERS) {
        ESP_LOGW(TAG, "Няма запазени филтри или броят е невалиден (%d)", filter_count);
        filter_count = 0;
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }

    size_t size = filter_count * sizeof(int);
    err = nvs_get_blob(nvs_handle, "type_filters", type_filters, &size);
    size = filter_count * sizeof(double);
    err |= nvs_get_blob(nvs_handle, "Hz", Hz, &size);
    err |= nvs_get_blob(nvs_handle, "dB", dB, &size);
    err |= nvs_get_blob(nvs_handle, "Qd", Qd, &size);
    uint8_t b=0, p=0;
	err |= nvs_get_u8(nvs_handle, "bypass_state", &b);
	err |= nvs_get_u8(nvs_handle, "phase_state", &p);
	bypass_state = b ? true : false;
	phase_state  = p ? true : false;


    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        for (int i = 0; i < filter_count; i++) {
            filters[i].type = type_filters[i];
            filters[i].freq = (float)Hz[i];
            filters[i].gain = (float)dB[i];
            filters[i].q = (float)Qd[i];
        }
        ESP_LOGI(TAG, "Заредени %d филтъра от NVS:", filter_count);
        for (int i = 0; i < filter_count; i++) {
            ESP_LOGW(TAG, "Filter %d: Type=%d, Freq=%.2f Hz, Gain=%.2f dB, Q=%.2f",
                     i + 1, filters[i].type, filters[i].freq, filters[i].gain, filters[i].q);
        }
         ESP_LOGW(TAG, "bypass_state = %s", bypass_state ? "true" : "false");
         ESP_LOGW(TAG, "phase_state  = %s", phase_state ? "true" : "false");
    } else {
        ESP_LOGE(TAG, "Грешка при зареждане на данни за филтрите: %s", esp_err_to_name(err));
    }

    return err;
}

/* --- Save / Load Coeffs --- */
esp_err_t save_coefficients() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при отваряне на NVS: %s", esp_err_to_name(err));
        return err;
    }

    if (coeff_count < 40 || coeff_count > MAX_COEFFS) {
        ESP_LOGE(TAG, "Некоректен coeff_count: %d", coeff_count);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    }

    err = nvs_set_i32(nvs_handle, "coeff_count", coeff_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Грешка при запис на coeff_count: %s", esp_err_to_name(err));
    } else {
        err = nvs_set_blob(nvs_handle, "coeffs", coeffs, coeff_count * sizeof(float));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Грешка при запис на коефициентите в NVS: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Запазени %d коефициента.", coeff_count);
            nvs_commit(nvs_handle);
        }
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_coefficients() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open(read) failed for coeffs: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_i32(nvs_handle, "coeff_count", &coeff_count);
    if (err != ESP_OK || coeff_count <= 0 || coeff_count > MAX_COEFFS) {
        ESP_LOGW(TAG, "Няма записани коефициенти или броят е невалиден (%d)", coeff_count);
        coeff_count = 0;
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }

    size_t required_size = coeff_count * sizeof(float);
    err = nvs_get_blob(nvs_handle, "coeffs", coeffs, &required_size);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Заредени %d коефициента.", coeff_count);
        for (int i = 0; i < coeff_count && i < 8; i++) { // кратко логване на първите 8
            ESP_LOGD(TAG, "coeff[%d] = %f", i, coeffs[i]);
        }
    } else {
        ESP_LOGE(TAG, "Грешка при четене на коефициентите: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

/* --- Handlers --- */

/* setDevice: записва дали да показваш EQ или DSP след рестарт */
esp_err_t set_device_handler(httpd_req_t *req) {
    char tmp[64];
    int ret = httpd_req_recv(req, tmp, sizeof(tmp) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    tmp[ret] = '\0';
    cJSON *json = cJSON_Parse(tmp);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *is_eq_item = cJSON_GetObjectItem(json, "is_eq");
    if (cJSON_IsBool(is_eq_item)) {
        save_device_type(cJSON_IsTrue(is_eq_item));
    }
    cJSON_Delete(json);
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
    return ESP_OK;
}

/* setFilters: записва филтрите (EQ) */
esp_err_t set_filter_handler(httpd_req_t *req) {
    int r = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (r <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
        return ESP_FAIL;
    }
    buf[r] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *filters_json = cJSON_GetObjectItem(json, "filters");
    if (!cJSON_IsArray(filters_json)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "filters array expected");
        return ESP_FAIL;
    }

    // четене на bypass и phase
   cJSON *bypass = cJSON_GetObjectItem(json, "bypass");
    cJSON *phase = cJSON_GetObjectItem(json, "phase");
    bypass_state = (bypass && cJSON_IsBool(bypass)) ? cJSON_IsTrue(bypass) : false;
    phase_state = (phase && cJSON_IsBool(phase)) ? cJSON_IsTrue(phase) : false;

    filter_count = 0;
    int array_size = cJSON_GetArraySize(filters_json);
    for (int i = 0; i < array_size && i < MAX_FILTERS; i++) {
        cJSON *fi = cJSON_GetArrayItem(filters_json, i);
        if (!cJSON_IsObject(fi)) continue;

        cJSON *type = cJSON_GetObjectItem(fi, "type");
        cJSON *freq = cJSON_GetObjectItem(fi, "freq");
        cJSON *gain = cJSON_GetObjectItem(fi, "gain");
        cJSON *q    = cJSON_GetObjectItem(fi, "q");

        if (cJSON_IsNumber(type) && cJSON_IsNumber(freq) &&
            cJSON_IsNumber(gain) && cJSON_IsNumber(q)) {
            filters[filter_count].type = type->valueint;
            filters[filter_count].freq = (float)freq->valuedouble;
            filters[filter_count].gain = (float)gain->valuedouble;
            filters[filter_count].q    = (float)q->valuedouble;

            type_filters[filter_count] = filters[filter_count].type;
            Hz[filter_count]           = filters[filter_count].freq;
            dB[filter_count]           = filters[filter_count].gain;
            Qd[filter_count]           = filters[filter_count].q;

            filter_count++;
        }
    }

    cJSON_Delete(json);

    esp_err_t err = save_filters();
    if (err == ESP_OK) {
        httpd_resp_send(req, "Filters saved, ESP will restart!", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
        return ESP_OK;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save filters");
        return ESP_FAIL;
    }
}


/* getFilters: връща JSON масив с филтрите (array of objects) */
esp_err_t get_filters_handler(httpd_req_t *req) {
    // Създаваме масив с филтрите
    cJSON *filters_arr = cJSON_CreateArray();
    for (int i = 0; i < filter_count; i++) {
        cJSON *f = cJSON_CreateObject();
        cJSON_AddNumberToObject(f, "type", type_filters[i]);
        cJSON_AddNumberToObject(f, "freq", Hz[i]);
        cJSON_AddNumberToObject(f, "gain", dB[i]);
        cJSON_AddNumberToObject(f, "q", Qd[i]);
        cJSON_AddItemToArray(filters_arr, f);
    }

    // Създаваме коренов обект
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "filters", filters_arr);
    cJSON_AddBoolToObject(root, "bypass", bypass_state);
    cJSON_AddBoolToObject(root, "phase", phase_state);

    // Превръщаме в JSON string
    char *response = cJSON_Print(root);
    if (!response) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create JSON");
        return ESP_FAIL;
    }

    // Изпращаме отговора
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);

    // Освобождаваме памет
    free(response);
    cJSON_Delete(root);

    return ESP_OK;
}


/* setCoeffs: запис на коефициенти (DSP) */
esp_err_t set_coeffs_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Получена POST заявка (setCoeffs) с дължина %d", req->content_len);
    int total_len = req->content_len;
    if (total_len > MAX_POST_SIZE) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }
    int received = 0;
    while (received < total_len) {
        int ret = httpd_req_recv(req, buf + received, total_len - received);
        if (ret <= 0) {
            ESP_LOGE(TAG, "Грешка при получаване: %d", ret);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Receive error");
            return ESP_FAIL;
        }
        received += ret;
    }
    buf[received] = '\0';
    ESP_LOGI(TAG, "Получени сурови JSON данни: %s", buf);
    strncpy(json_buffer, buf, sizeof(json_buffer) - 1);

    cJSON *json = cJSON_Parse(json_buffer);
    if (!json) {
        ESP_LOGE(TAG, "Невалиден JSON: %s", json_buffer);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *coeff_array = cJSON_GetObjectItem(json, "coeffs");
    if (!coeff_array || !cJSON_IsArray(coeff_array)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Expected array 'coeffs'");
        return ESP_FAIL;
    }

    int num = cJSON_GetArraySize(coeff_array);
    if (num > MAX_COEFFS) {
        ESP_LOGW(TAG, "Прекалено много коефициенти, ограничаваме до %d", MAX_COEFFS);
        num = MAX_COEFFS;
    }

    for (int i = 0; i < num; i++) {
        cJSON *it = cJSON_GetArrayItem(coeff_array, i);
        if (cJSON_IsNumber(it)) {
            coeffs[i] = (float)it->valuedouble;
        } else {
            coeffs[i] = 0.0f;
        }
    }
    coeff_count = num;
    cJSON_Delete(json);

    esp_err_t err = save_coefficients();
    if (err == ESP_OK) {
        httpd_resp_send(req, "Coefficients saved, restarting...", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
        return ESP_OK;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save coeffs");
        return ESP_FAIL;
    }
}

/* getCoeffs: връща JSON { "coeffs": [...] } */
esp_err_t get_coeffs_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < coeff_count; i++) {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber((double)coeffs[i]));
    }
    cJSON_AddItemToObject(root, "coeffs", arr);
    char *response = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    free(response);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Landing GET: избира правилната страница според NVS запис (is_eq) */
esp_err_t get_landing_handler(httpd_req_t *req) {
    bool stored_eq = false;
    if (load_device_type(&stored_eq) == ESP_OK) {
        is_eq = stored_eq;
        // вече имаме запис → показваме правилната страница
        if (is_eq) {
            return httpd_resp_send(req, html_page_eq, HTTPD_RESP_USE_STRLEN);
        } else {
            return httpd_resp_send(req, html_page_dsp, HTTPD_RESP_USE_STRLEN);
        }
    } else {
        // Няма запис → показваме landing page
        return httpd_resp_send(req, html_landing_page, HTTPD_RESP_USE_STRLEN);
    }
}

/* --- Start webserver: регистрирам всички handlers --- */
httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/", .method = HTTP_GET, .handler = get_landing_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/setDevice", .method = HTTP_POST, .handler = set_device_handler });

        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/getFilters", .method = HTTP_GET, .handler = get_filters_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/setFilters", .method = HTTP_POST, .handler = set_filter_handler });

        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/getCoeffs", .method = HTTP_GET, .handler = get_coeffs_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/setCoeffs", .method = HTTP_POST, .handler = set_coeffs_handler });
    } else {
        ESP_LOGE(TAG, "httpd_start failed");
    }

    return server;
}

/* --- WiFi softAP --- */
void wifi_init_softap() {
    char ssid_name[32];

    strcpy(ssid_name, "ESP32_DSP_SETTINGS");
    

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = { 0 }; // нулирай
    strncpy((char *)wifi_config.ap.ssid, ssid_name, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, "12345678", sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}

/* --- web_server init: load NVS data then start AP+webserver --- */
esp_err_t web_server() {
    ESP_LOGI(TAG, "Инициализация на NVS...");
    esp_err_t errnvs = nvs_flash_init();
    if (errnvs == ESP_OK) ESP_LOGW(TAG, "NVS OK...");

    /* Опитваме да заредим и coeffs, и filters (ако има) */
    if (load_coefficients() == ESP_OK) {
        ESP_LOGI(TAG, "Loaded coeffs from NVS: %d", coeff_count);
        return 0;
    } else {
        ESP_LOGW(TAG, "No coeffs found or error");
    }

    if (load_filters() == ESP_OK) {
        ESP_LOGI(TAG, "Loaded filters from NVS: %d", filter_count);
        return 0;
    } else {
        ESP_LOGW(TAG, "No filters found or error");
    }

    /* Стартираме AP и webserver */
    ESP_LOGI(TAG, "Стартиране на Wi-Fi AP...");
    wifi_init_softap();
    ESP_LOGI(TAG, "Стартиране на уеб сървъра...");
    start_webserver();
    ESP_LOGI("MEMORY", "Free internal SRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    return ESP_OK;
}

