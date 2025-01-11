#pragma once

#include <esp_http_server.h>

httpd_handle_t start_webserver(ssids_t *ssids, uint16_t scan_count);