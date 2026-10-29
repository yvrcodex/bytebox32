#include <Arduino.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "discovery.h"
#include "ota.h"
#include "provisioning.h"
#include "sdcard.h"
#include "settings.h"
#include "status_led.h"
#include "web.h"
#include "wifi_station.h"

#define SUPERVISOR_PERIOD_MS 100

static const char *TAG = "main";

// Runs on every (re)connection; ArduinoOTA must be started only once.
static void on_online()
{
    static bool ota_started = false;

    if (!ota_started) {
        ota_started = ota_init();
    }
}

// Holding BOOT for BOOT_BUTTON_HOLD_MS enables configuration mode.
static void poll_boot_button()
{
    static uint32_t held_ms = 0;

    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {  // Active low
        held_ms += SUPERVISOR_PERIOD_MS;
        if (held_ms == BOOT_BUTTON_HOLD_MS) {
            provisioning_set_config_mode(true);
        }
    } else {
        held_ms = 0;
    }
}

// Priority: configuration > Wi-Fi > other errors > OK.
static status_led_t current_status()
{
    if (provisioning_config_mode() || !settings_has_wifi()) {
        return STATUS_LED_CONFIG;
    }

    if (provisioning_has_wifi_error()) {
        return STATUS_LED_WIFI_ERROR;
    }

    if (!provisioning_is_online() || !sdcard_is_mounted()) {
        return STATUS_LED_ERROR;
    }

    return STATUS_LED_OK;
}

void setup()
{
    status_led_set(STATUS_LED_ERROR);  // Until the device is fully up.
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    if (!settings_init()) {
        return;
    }

    sdcard_init();  // Optional: the device keeps running without a card.

    wifi_station_init();
    discovery_init();  // After Wi-Fi init: needs the network interfaces.

    if (!web_start()) {
        ESP_LOGE(TAG, "Web server failed to start");
    }

    if (!provisioning_start(on_online)) {
        ESP_LOGE(TAG, "Provisioning failed to start");
    }
}

// Supervisor: BOOT button and status LED.
void loop()
{
    static TickType_t last_wake = xTaskGetTickCount();

    poll_boot_button();
    status_led_set(current_status());

    xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SUPERVISOR_PERIOD_MS));
}
