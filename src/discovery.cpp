#include "discovery.h"

#include <ESPmDNS.h>
#include <esp_log.h>

#include "config.h"

#define HTTP_PORT 80

static const char *TAG = "discovery";

bool discovery_init()
{
    if (!MDNS.begin(DEVICE_HOSTNAME)) {
        ESP_LOGE(TAG, "Failed to start mDNS");
        return false;
    }

    MDNS.addService("http", "tcp", HTTP_PORT);

    ESP_LOGI(TAG, "Responding as %s.local", DEVICE_HOSTNAME);
    return true;
}
