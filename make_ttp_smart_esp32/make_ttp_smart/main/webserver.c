#include <esp_http_server.h>
#include "esp_log.h"

#include "esp_spiffs.h"
#include "cJSON.h"


static const char *TAG = "HTTP_SERVER";


static esp_err_t root_get_handler(httpd_req_t *req) {
    char buf[128];

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

static esp_err_t picocss_get_handler(httpd_req_t *req) {
    char buf[128];

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

// Start the web server
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &picocss);
        httpd_register_uri_handler(server, &favicon);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
    return server;
}