#include <esp_http_server.h>
#include "esp_log.h"

#include "esp_spiffs.h"
#include "cJSON.h"

#include "wifi.h"
#include "esp_system.h"

#include "config.h"
#include "shtc3.h"

static const char *TAG = "HTTP_SERVER";
static const ssids_t *_ssids;
static uint16_t _scan_count;

static esp_err_t root_get_handler(httpd_req_t *req) {
    char buf[512];

    FILE* file = fopen("/storage/index.html", "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
  
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), file)) > 0) {
        httpd_resp_send_chunk(req, buf, bytes_read);
    }

    fclose(file);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Handler for the root url of the server
static httpd_uri_t root = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = root_get_handler,
    .user_ctx = NULL
};

static esp_err_t root_post_handler(httpd_req_t *req) {
    char buf[512];
    FILE* file = fopen("/storage/reset.html", "r");
      
    char content[256];
    int total_len = req->content_len;
    int received = 0;

    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    while (received < total_len) {
        int ret = httpd_req_recv(req, content + received, sizeof(content) - 1 - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            ESP_LOGE(TAG, "Error receiving data");
            return ESP_FAIL;
        }
        received += ret;
    }
    content[received] = '\0';

    cJSON *json = cJSON_Parse(content);

    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *wifi_password = cJSON_GetObjectItem(json, "wifi_password");
    cJSON *humidity_threshold = cJSON_GetObjectItem(json, "humidity_threshold");

    if (cJSON_IsString(wifi_password) && strlen(wifi_password->valuestring) > 1 && cJSON_IsString(ssid)) {
        config_wifi_t wifi_config;

        strncpy(wifi_config.ssid, ssid->valuestring, sizeof(wifi_config.ssid));
        strncpy(wifi_config.password, wifi_password->valuestring, sizeof(wifi_config.password));

        config_set_wifi(wifi_config);
    }
    
    if (cJSON_IsNumber(humidity_threshold)) {
        config_set_humidty_threshold(humidity_threshold->valueint);
    }

    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), file)) > 0) {
        httpd_resp_send_chunk(req, buf, bytes_read);
    }

    fclose(file);

    httpd_resp_send_chunk(req, NULL, 0);

    cJSON_Delete(json); 
    ESP_LOGI(TAG, "Restarting...");
    esp_restart();
    return ESP_OK;
}

// Handler for the root url of the server
static httpd_uri_t root_post = {
    .uri      = "/",
    .method   = HTTP_POST,
    .handler  = root_post_handler,
    .user_ctx = NULL
};

static esp_err_t picocss_get_handler(httpd_req_t *req) {
    char buf[512];

    FILE* file = fopen("/storage/pico.min.css", "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "text/css");
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), file)) > 0) {
        httpd_resp_send_chunk(req, buf, bytes_read);
    }

    fclose(file);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


// Handler for the root url of the server
static httpd_uri_t picocss = {
    .uri      = "/pico.min.css",
    .method   = HTTP_GET,
    .handler  = picocss_get_handler,
    .user_ctx = NULL
};

static esp_err_t favicon_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/x-icon");

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}


// Handler for the root url of the server
static httpd_uri_t favicon = {
    .uri      = "/favicon.ico",
    .method   = HTTP_GET,
    .handler  = favicon_get_handler,
    .user_ctx = NULL
};


static esp_err_t json_get_handler(httpd_req_t *req) {
    shtc3_t shtc3 = shtc3_static_read();

    // Neues JSON-Objekt erstellen
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        ESP_LOGE(TAG, "Error creating JSON object");
        return ESP_ERR_NOT_FINISHED; 
    }

    // Felder zum JSON-Objekt hinzufügen
    cJSON_AddNumberToObject(json, "humidity", shtc3.humidity);
    cJSON_AddNumberToObject(json, "temperature", shtc3.temperature);
    cJSON_AddNumberToObject(json, "humidity_threshold", config_get_humidity_threshold());

    cJSON *array = cJSON_AddArrayToObject(json, "ssids");
    for (int i = 0; i < _scan_count; i++) {
        cJSON_AddItemToArray(array, cJSON_CreateString(_ssids[i].ssid));
    }
    
    httpd_resp_set_type(req, "application/json");

    httpd_resp_send(req, cJSON_Print(json), strlen(cJSON_Print(json)));

    cJSON_Delete(json);
    return ESP_OK;
}


// Handler for the root url of the server
static httpd_uri_t json = {
    .uri      = "/json",
    .method   = HTTP_GET,
    .handler  = json_get_handler,
    .user_ctx = NULL
};


// Start the web server
httpd_handle_t start_webserver(ssids_t *ssids, uint16_t scan_count)
{
    _ssids = ssids;
    _scan_count = scan_count;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &root_post);
        httpd_register_uri_handler(server, &picocss);
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &json);

    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
    return server;
}