// Over-the-air firmware updates via ArduinoOTA.
#ifndef OTA_H
#define OTA_H

// Starts the OTA service task. Call after Wi-Fi is connected.
// Returns true on success.
bool ota_init();

#endif  // OTA_H
