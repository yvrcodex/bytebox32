#include "status_led.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

static rgb_t status_color(status_led_t status)
{
    switch (status) {
    case STATUS_LED_OK:         return {0, 0, STATUS_LED_LEVEL_OK};
    case STATUS_LED_ERROR:      return {STATUS_LED_LEVEL_ERROR, 0, 0};
    case STATUS_LED_WIFI_ERROR: return {STATUS_LED_LEVEL_BLINK, STATUS_LED_LEVEL_BLINK / 3, 0};
    case STATUS_LED_CONFIG:     return {STATUS_LED_LEVEL_BLINK, 0, STATUS_LED_LEVEL_BLINK};
    case STATUS_LED_OFF:        break;
    }

    return {0, 0, 0};
}

static bool status_blinks(status_led_t status)
{
    return status == STATUS_LED_WIFI_ERROR || status == STATUS_LED_CONFIG;
}

void status_led_set(status_led_t status)
{
    static bool  initialized = false;
    static rgb_t current     = {0, 0, 0};

    rgb_t next = status_color(status);

    if (status_blinks(status)) {
        const TickType_t phase = xTaskGetTickCount() / pdMS_TO_TICKS(STATUS_LED_BLINK_MS);
        if (phase & 1) {
            next = {0, 0, 0};
        }
    }

    // Write only on change: each write is an RMT transaction.
    if (initialized && next.r == current.r && next.g == current.g && next.b == current.b) {
        return;
    }

    rgbLedWrite(STATUS_LED_PIN, next.r, next.g, next.b);

    current     = next;
    initialized = true;
}
