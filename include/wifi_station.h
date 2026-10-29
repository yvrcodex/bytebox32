// Wi-Fi station (STA) management. Non-blocking.
#ifndef WIFI_STATION_H
#define WIFI_STATION_H

typedef enum {
    WIFI_STATION_IDLE,
    WIFI_STATION_CONNECTING,
    WIFI_STATION_CONNECTED,
    WIFI_STATION_NO_AP,        // SSID not found
    WIFI_STATION_AUTH_FAILED,  // Wrong password
    WIFI_STATION_FAILED,       // Any other failure
} wifi_station_state_t;

// Configures the STA interface. Call once at boot.
void wifi_station_init();

// Starts a connection attempt and returns immediately.
bool wifi_station_connect(const char *ssid, const char *password);

// Current connection state. Safe to call from any task.
wifi_station_state_t wifi_station_get_state();

// Human-readable state name for logs and the portal.
const char *wifi_station_state_name(wifi_station_state_t state);

#endif  // WIFI_STATION_H
