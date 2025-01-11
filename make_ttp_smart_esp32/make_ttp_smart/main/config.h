#pragma once 

#include "stdint.h"

typedef struct {
    char ssid[32];
    char password[64];
} config_wifi_t;

uint16_t config_get_humidity_threshold();
void config_set_humidty_threshold(uint16_t humidty_threshold);

config_wifi_t config_get_wifi();
void config_set_wifi(config_wifi_t wifi_config);