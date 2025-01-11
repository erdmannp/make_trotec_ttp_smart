#ifndef WIFI_H
#define WIFI_H

typedef struct ssids {
    char ssid[33];
} ssids_t;

typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_IN_AP_MODE,
    WIFI_STATE_IN_CLIENT_MODE
} wifi_state_t;

void wifi_scan(ssids_t *ssids, uint16_t *scan_count);
void wifi_init_softap(void);
void wifi_deinit_(void);
void wifi_init_(void);
void wifi_connect(void);
wifi_state_t wifi_get_state(void);

#endif // WIFI_H