// Onboard WS2812 status LED.
#ifndef STATUS_LED_H
#define STATUS_LED_H

typedef enum {
    STATUS_LED_OFF,
    STATUS_LED_OK,          // Steady blue
    STATUS_LED_ERROR,       // Steady red
    STATUS_LED_WIFI_ERROR,  // Blinking orange
    STATUS_LED_CONFIG,      // Blinking purple
} status_led_t;

// Shows status. Call periodically (faster than STATUS_LED_BLINK_MS) to blink.
void status_led_set(status_led_t status);

#endif  // STATUS_LED_H
