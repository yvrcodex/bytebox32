#include "settings.h"

#include <Preferences.h>
#include <esp_log.h>
#include <string.h>

#define SETTINGS_NAMESPACE "bytebox32"
#define KEY_WIFI_SSID      "wifi_ssid"
#define KEY_WIFI_PASSWORD  "wifi_pass"

static const char *TAG = "settings";

static Preferences prefs;

bool settings_init()
{
    if (!prefs.begin(SETTINGS_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open NVS namespace");
        return false;
    }

    return true;
}

bool settings_has_wifi()
{
    // SSID is written last, so its presence implies a complete record.
    return prefs.isKey(KEY_WIFI_SSID);
}

bool settings_get_wifi(char *ssid, size_t ssid_size,
                       char *password, size_t password_size)
{
    if (!settings_has_wifi()) {
        return false;
    }

    // getString() returns 0 if the key is missing or the buffer is too small.
    if (prefs.getString(KEY_WIFI_SSID, ssid, ssid_size) == 0 ||
        prefs.getString(KEY_WIFI_PASSWORD, password, password_size) == 0) {
        ESP_LOGE(TAG, "Failed to read Wi-Fi credentials");
        return false;
    }

    return true;
}

bool settings_set_wifi(const char *ssid, const char *password)
{
    if (ssid == nullptr || password == nullptr) {
        return false;
    }

    const size_t ssid_len     = strlen(ssid);
    const size_t password_len = strlen(password);

    if (ssid_len == 0 || ssid_len >= SETTINGS_SSID_SIZE ||
        password_len >= SETTINGS_PASSWORD_SIZE) {
        ESP_LOGE(TAG, "Invalid credential length");
        return false;
    }

    // Password first: a power loss mid-write never leaves a new SSID
    // paired with an old password.
    if (prefs.putString(KEY_WIFI_PASSWORD, password) != password_len ||
        prefs.putString(KEY_WIFI_SSID, ssid) != ssid_len) {
        ESP_LOGE(TAG, "Failed to write Wi-Fi credentials");
        return false;
    }

    ESP_LOGI(TAG, "Wi-Fi credentials saved for \"%s\"", ssid);
    return true;
}
