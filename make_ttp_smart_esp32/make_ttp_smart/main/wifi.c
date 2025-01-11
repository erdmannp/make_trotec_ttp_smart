#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"

#include <string.h>

#include "wifi.h"

#include "config.h"

const static char *TAG = "wifi";
const uint16_t DEFAULT_SCAN_LIST_SIZE = 15;

static wifi_state_t wifi_state = WIFI_STATE_DISCONNECTED;

wifi_state_t wifi_get_state(void)
{
    return wifi_state;
}


void wifi_scan(ssids_t *ssids, uint16_t *scan_count)
{
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = (*scan_count > DEFAULT_SCAN_LIST_SIZE)? DEFAULT_SCAN_LIST_SIZE : *scan_count;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));
    ESP_LOGI(TAG, "%d", *scan_count);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_scan_start(NULL, true);

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    *scan_count = ap_count;
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
    for (int i = 0; i < number; i++) {
        strncpy(ssids[i].ssid, (char *)ap_info[i].ssid, 33);
    }
}



void wifi_init_softap(void)
{
    #define EXAMPLE_ESP_WIFI_SSID "TTP5-Access-Point"

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = 5,
            .password = "",
            .max_connection = 1,
            .authmode = WIFI_AUTH_OPEN,

            .pmf_cfg = {
                .required = true,
            },
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_state = WIFI_STATE_IN_AP_MODE;
}

// Initialize Wi-Fi
void wifi_connect(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    config_wifi_t config = config_get_wifi();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    memcpy(wifi_config.sta.ssid, config.ssid, strlen(config.ssid));
    memcpy(wifi_config.sta.password, config.password, strlen(config.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    ESP_ERROR_CHECK(esp_wifi_connect());

    wifi_state = WIFI_STATE_IN_CLIENT_MODE;
}

void wifi_deinit_(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    wifi_state = WIFI_STATE_DISCONNECTED;
}

void wifi_init_(void)
{
    config_wifi_t config = config_get_wifi();

    ESP_LOGI(TAG, "connecting to Wifi: %s pass: %s", config.ssid, config.password);

    if (strlen(config.ssid) > 0 && strlen(config.password) > 0) {
        wifi_connect();
    } else {
        wifi_init_softap();
    }
}