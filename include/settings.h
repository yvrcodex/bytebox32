// Persistent device settings stored in NVS.
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stddef.h>

#define SETTINGS_SSID_SIZE     33  // 32 chars + NUL (802.11 limit)
#define SETTINGS_PASSWORD_SIZE 65  // 64 chars + NUL (WPA2 PSK limit)

// Opens the settings namespace. Call once at boot.
bool settings_init();

// Returns true if Wi-Fi credentials are stored.
bool settings_has_wifi();

// Reads stored credentials. Buffers should be at least
// SETTINGS_SSID_SIZE and SETTINGS_PASSWORD_SIZE bytes.
bool settings_get_wifi(char *ssid, size_t ssid_size,
                       char *password, size_t password_size);

// Stores new credentials. An empty password means an open network.
bool settings_set_wifi(const char *ssid, const char *password);

#endif  // SETTINGS_H
