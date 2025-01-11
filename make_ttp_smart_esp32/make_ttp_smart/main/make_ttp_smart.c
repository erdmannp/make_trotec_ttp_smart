#include <stdio.h>
#include "esp_wifi.h"

#include "wifi.h"
#include "webserver.h"

#include "nvs_flash.h"

#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "esp_task_wdt.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "shtc3.h"

TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t webserverTaskHandle = NULL;
TaskHandle_t shtc3_task = NULL;

typedef struct webserverTaskData {
    ssids_t *ssids;
    uint16_t scan_count;
} webserverTaskData_t;


static const char *TAG = "main";


static void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",      // Mount-Punkt
        .partition_label = NULL,    // Standard-Partition
        .max_files = 5,             // Max. geöffnete Dateien
        .format_if_mount_failed = true // Formatieren bei Fehler
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS initialisieren fehlgeschlagen (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS-Info abrufen fehlgeschlagen (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS erfolgreich gemountet");
        ESP_LOGI(TAG, "Gesamter Speicher: %d, Verwendet: %d", total, used);
    }
}

void wifiTask(void *arg)
{
    const char *taskTag = "wifiTask";
    ESP_LOGI(taskTag, "wifiTask started");
    esp_netif_ip_info_t ip_info;
    const uint8_t no_of_retries = 10;
    uint8_t retries = no_of_retries;

    while (1)
    {

        if (wifi_get_state() != WIFI_STATE_IN_CLIENT_MODE) {
            vTaskDelete(NULL);
            return;
        }
        
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif != NULL) {
            esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
            if (ret == ESP_OK) {
                if (ip_info.ip.addr == 0) {
                    ESP_LOGI(taskTag, "IP address not set yet");
                    retries--;
                } else {
                    retries = no_of_retries;
                }
            } else {
                ESP_LOGE(taskTag, "Failed to get STA IP address");
            }
        }
        if (retries == 0) {
            wifi_deinit_();
            wifi_init_();
            retries = no_of_retries;
        }
        vTaskDelay(100);
    }
}   


void app_main(void)
{
    uint16_t scan_count = 13;
    ssids_t *ssids = malloc(sizeof(ssids_t) * scan_count);
    
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_spiffs();
    
    wifi_scan(ssids, &scan_count);

    wifi_init_();
    xTaskCreate(shtc3_task_f, "SHTC3 Task", 4096, NULL, 5, &shtc3_task);
    xTaskCreate(wifiTask, "wifiTask", 4096, NULL, 5, &wifiTaskHandle);

    start_webserver(ssids, scan_count);


}

