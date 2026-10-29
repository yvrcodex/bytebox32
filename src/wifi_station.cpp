#include "wifi_station.h"

#include <WiFi.h>
#include <atomic>
#include <esp_log.h>

#include "config.h"

static const char *TAG = "wifi_station";

// Written by the Wi-Fi event task, read by any task.
static std::atomic<wifi_station_state_t> state{WIFI_STATION_IDLE};

static wifi_station_state_t reason_to_state(uint8_t reason)
{
    switch (reason) {
    case WIFI_REASON_NO_AP_FOUND:
        return WIFI_STATION_NO_AP;
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        return WIFI_STATION_AUTH_FAILED;
    default:
        return WIFI_STATION_FAILED;
    }
}

static void on_wifi_event(arduino_event_id_t event, arduino_event_info_t info)
{
    switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        state = WIFI_STATION_CONNECTED;
        ESP_LOGI(TAG, "Connected, IP: %s", WiFi.localIP().toString().c_str());
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
        const uint8_t reason = info.wifi_sta_disconnected.reason;

        // ASSOC_LEAVE is our own disconnect before a new attempt.
        if (reason == WIFI_REASON_ASSOC_LEAVE) {
            break;
        }

        state = reason_to_state(reason);
        ESP_LOGW(TAG, "Disconnected: %s (reason %u)",
                 wifi_station_state_name(state), reason);
        break;
    }

    default:
        break;
    }
}

void wifi_station_init()
{
    WiFi.persistent(false);       // Credentials live in our own NVS namespace.
    WiFi.setAutoReconnect(false); // Retries are owned by the caller.
    WiFi.setHostname(DEVICE_HOSTNAME);  // Must precede mode().
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(on_wifi_event);
}

bool wifi_station_connect(const char *ssid, const char *password)
{
    state = WIFI_STATION_CONNECTING;

    if (WiFi.begin(ssid, password) == WL_CONNECT_FAILED) {
        state = WIFI_STATION_FAILED;
        ESP_LOGE(TAG, "Invalid connection parameters");
        return false;
    }

    ESP_LOGI(TAG, "Connecting to \"%s\"", ssid);
    return true;
}

wifi_station_state_t wifi_station_get_state()
{
    return state;
}

const char *wifi_station_state_name(wifi_station_state_t state)
{
    switch (state) {
    case WIFI_STATION_IDLE:        return "idle";
    case WIFI_STATION_CONNECTING:  return "connecting";
    case WIFI_STATION_CONNECTED:   return "connected";
    case WIFI_STATION_NO_AP:       return "network not found";
    case WIFI_STATION_AUTH_FAILED: return "wrong password";
    case WIFI_STATION_FAILED:      return "connection failed";
    }

    return "unknown";
}
