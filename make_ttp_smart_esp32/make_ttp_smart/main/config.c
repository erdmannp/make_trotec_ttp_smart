#include "config.h"
#include <stdio.h>
#include <string.h>

#include "cJSON.h"

#include "esp_log.h"
#include<errno.h>

const char *humidity_config_fn = "/storage/humidity_threshold.config";
const char *wifi_config_fn = "/storage/wifi.config";

const char *TAG = "config";

static void read_humidity_config(cJSON **ret) {
    FILE* file = fopen(humidity_config_fn, "r");
    char buf[128];
    
    if (!file) {
        return;
    }

    fread(buf, 1, sizeof(buf), file);
    fclose(file);

    *ret = cJSON_Parse(buf);
    if (*ret == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            ESP_LOGE(TAG, "Error: %s", error_ptr); 
        } 
        return; 
    }
}

uint16_t config_get_humidity_threshold() {
    cJSON *json = NULL;
    uint16_t ret = 0;

    read_humidity_config(&json);

    if (json == NULL) {
        ESP_LOGE(TAG, "JSON object is null");
        return 0;
    }

    cJSON *ht = cJSON_GetObjectItemCaseSensitive(json, "humidity_threshold"); 

    if (cJSON_IsNumber(ht) && (ht != NULL)) { 
        ret = ht->valueint;

    } else {
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "cJSON parsing Error: %s", error_ptr);
        }
    }
    cJSON_Delete(json);

    return ret;
}

void config_set_humidty_threshold(uint16_t humidty_threshold) {
    cJSON *json = NULL;
    FILE *f;

    read_humidity_config(&json);
    cJSON *ht = cJSON_GetObjectItemCaseSensitive(json, "humidity_threshold"); 
    cJSON_SetIntValue(ht, humidty_threshold);

    f = fopen(humidity_config_fn, "w");
    fwrite(cJSON_Print(json), strlen(cJSON_Print(json)), 1, f);
    ESP_LOGI(TAG, "humidity threshold written to config: %s", cJSON_Print(json));
    fflush(f); 
    fclose(f);
    
    cJSON_Delete(json);
}

static void read_wifi_config(cJSON **ret) {
    FILE* file = fopen(wifi_config_fn, "r");
    char buf[256];
    
    if (!file) {
        return;
    }

    fread(buf, 1, sizeof(buf), file);
    fclose(file);

    *ret = cJSON_Parse(buf);
    if (*ret == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            ESP_LOGE(TAG, "Error: %s", error_ptr); 
        } 
        return; 
    }
}

config_wifi_t config_get_wifi() {
    cJSON *json = NULL;
    config_wifi_t cfg = {0};

    read_wifi_config(&json);
    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (    
        cJSON_IsString(ssid) && (ssid != NULL) && 
        cJSON_IsString(password) && (password != NULL)) 
    { 
        strncpy(cfg.ssid, ssid->valuestring, strlen(ssid->valuestring));
        strncpy(cfg.password, password->valuestring, strlen(password->valuestring));

    }
    ESP_LOGI(TAG, "ssid: %s, password: %s", cfg.ssid, cfg.password);

    cJSON_Delete(json);
    
    return cfg;
}


void config_set_wifi(config_wifi_t wifi_config){
    FILE *f;
    cJSON *json = NULL;
    size_t bytes_written = 0;

    read_wifi_config(&json);

    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

    cJSON_SetValuestring(ssid, wifi_config.ssid);
    cJSON_SetValuestring(password, wifi_config.password);

    ESP_LOGI(TAG, "Write wifi config: %s", cJSON_Print(json));

    f = fopen(wifi_config_fn, "w");

    if (f == NULL) {
        ESP_LOGE(TAG, "fopen failed with %s", strerror(errno));
    }

    bytes_written = fwrite(cJSON_Print(json), strlen(cJSON_Print(json)), 1, f);

    if (bytes_written == 0) {
        ESP_LOGE(TAG, "fwrite failed with %s", strerror(errno));
    }

    fflush(f);
    fclose(f);

    cJSON_Delete(json);
}