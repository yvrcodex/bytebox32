// Project-wide, non-secret configuration.
#ifndef CONFIG_H
#define CONFIG_H

#define DEVICE_HOSTNAME "bytebox32"
#define PORTAL_AP_SSID  "bytebox32"

// SD card (SDMMC, 1-bit bus)
#define SD_PIN_CLK 39
#define SD_PIN_CMD 38
#define SD_PIN_D0  40

// Onboard WS2812 status LED (GPIO48 on ESP32-S3 DevKitC/Freenove layouts)
// Brightness levels are 0-255.
#define STATUS_LED_PIN         48
#define STATUS_LED_LEVEL_OK    1    // Steady blue
#define STATUS_LED_LEVEL_ERROR 4    // Steady red
#define STATUS_LED_LEVEL_BLINK 8    // Blinking orange/purple
#define STATUS_LED_BLINK_MS    500  // Half period

// BOOT button: hold to enter configuration mode
#define BOOT_BUTTON_PIN     0
#define BOOT_BUTTON_HOLD_MS 3000

#endif  // CONFIG_H
