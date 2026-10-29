#include "provisioning.h"

#include <atomic>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "portal.h"
#include "settings.h"
#include "wifi_station.h"

#define PROV_TASK_STACK    4096
#define PROV_TASK_PRIORITY 1
#define PROV_TICK_MS       100
#define RETRY_DELAY_MS     3000
#define CONNECT_TIMEOUT_MS 20000

typedef enum {
    PROV_WAIT_CONFIG,  // No saved network
    PROV_CONNECT,
    PROV_CONNECTING,
    PROV_RETRY_WAIT,
    PROV_ONLINE,
} prov_state_t;

static const char *TAG = "provisioning";

static provisioning_cb_t online_cb;

// Set from other tasks (web, main), consumed by the provisioning task.
static std::atomic<bool> reconnect_requested{false};
static std::atomic<bool> config_mode{false};
static std::atomic<bool> online{false};
static std::atomic<bool> connect_failed{false};  // Since the last successful connection

static bool elapsed(TickType_t since, uint32_t ms)
{
    return (xTaskGetTickCount() - since) >= pdMS_TO_TICKS(ms);
}

static bool connect_saved()
{
    char ssid[SETTINGS_SSID_SIZE];
    char password[SETTINGS_PASSWORD_SIZE];

    if (!settings_get_wifi(ssid, sizeof(ssid), password, sizeof(password))) {
        return false;
    }

    return wifi_station_connect(ssid, password);
}

// Brings the setup AP up or down to match the desired state.
static void apply_ap_policy(bool needed)
{
    if (needed && !portal_is_running()) {
        portal_start();
    } else if (!needed && portal_is_running()) {
        portal_stop();
    }
}

static void provisioning_task(void *)
{
    TickType_t last_wake   = xTaskGetTickCount();
    TickType_t state_since = last_wake;
    bool new_credentials   = false;  // Current attempt uses freshly saved ones
    prov_state_t state;

    auto enter = [&](prov_state_t next) {
        state = next;
        state_since = xTaskGetTickCount();
    };

    if (settings_has_wifi()) {
        enter(PROV_CONNECT);
    } else {
        ESP_LOGW(TAG, "No Wi-Fi configured, setup AP enabled");
        enter(PROV_WAIT_CONFIG);
    }

    for (;;) {
        if (reconnect_requested.exchange(false)) {
            new_credentials = true;
            enter(PROV_CONNECT);
        }

        switch (state) {
        case PROV_WAIT_CONFIG:
            break;

        case PROV_CONNECT:
            if (connect_saved()) {
                enter(PROV_CONNECTING);
            } else {
                enter(settings_has_wifi() ? PROV_RETRY_WAIT : PROV_WAIT_CONFIG);
            }
            break;

        case PROV_CONNECTING: {
            const wifi_station_state_t wifi = wifi_station_get_state();

            if (wifi == WIFI_STATION_CONNECTED) {
                ESP_LOGI(TAG, "Online");
                online = true;
                connect_failed = false;

                // New credentials worked: configuration is done.
                if (new_credentials) {
                    new_credentials = false;
                    config_mode = false;
                }

                enter(PROV_ONLINE);

                if (online_cb != nullptr) {
                    online_cb();
                }
            } else if (wifi != WIFI_STATION_CONNECTING ||
                       elapsed(state_since, CONNECT_TIMEOUT_MS)) {
                ESP_LOGW(TAG, "Connect failed (%s), retrying in %d ms",
                         wifi_station_state_name(wifi), RETRY_DELAY_MS);
                connect_failed = true;
                enter(PROV_RETRY_WAIT);
            }
            break;
        }

        case PROV_RETRY_WAIT:
            if (elapsed(state_since, RETRY_DELAY_MS)) {
                enter(PROV_CONNECT);
            }
            break;

        case PROV_ONLINE:
            if (wifi_station_get_state() != WIFI_STATION_CONNECTED) {
                ESP_LOGW(TAG, "Connection lost, reconnecting");
                online = false;
                enter(PROV_CONNECT);
            }
            break;
        }

        apply_ap_policy(config_mode || state == PROV_WAIT_CONFIG || connect_failed);

        xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PROV_TICK_MS));
    }
}

bool provisioning_start(provisioning_cb_t on_online)
{
    online_cb = on_online;

    if (xTaskCreate(provisioning_task, "provisioning", PROV_TASK_STACK, nullptr,
                    PROV_TASK_PRIORITY, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return false;
    }

    return true;
}

void provisioning_reconnect()
{
    reconnect_requested = true;
}

void provisioning_set_config_mode(bool enabled)
{
    if (config_mode.exchange(enabled) != enabled) {
        ESP_LOGI(TAG, "Configuration mode %s", enabled ? "on" : "off");
    }
}

bool provisioning_config_mode()
{
    return config_mode;
}

bool provisioning_is_online()
{
    return online;
}

bool provisioning_has_wifi_error()
{
    return connect_failed;
}
