idf_component_register(SRCS "wifi.c" "make_ttp_smart.c" "webserver.c" "../cJSON/cJSON.c" "../cJSON/cJSON_Utils.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_https_server spiffs
                    INCLUDE_DIRS "." ../cJSON)

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)