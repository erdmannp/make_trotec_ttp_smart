#include <stdio.h>
#include "esp_wifi.h"

#include "wifi.h"
#include "webserver.h"

#include "nvs_flash.h"

#include "esp_spiffs.h"
#include "esp_log.h"


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

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_spiffs();

    wifi_scan();
    wifi_connect();

    start_webserver();
}

