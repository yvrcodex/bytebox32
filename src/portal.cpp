#include "portal.h"

#include <DNSServer.h>
#include <WiFi.h>
#include <atomic>
#include <esp_log.h>

#include "config.h"

#define DNS_PORT          53
#define WPA2_MIN_PASSWORD 8

static_assert(sizeof(AP_PASSWORD) > WPA2_MIN_PASSWORD,
              "AP_PASSWORD must have at least 8 characters (WPA2)");

static const char *TAG = "portal";

static DNSServer dns;  // Async (AsyncUDP): no polling task required.

static std::atomic<bool> running{false};

bool portal_start()
{
    if (running) {
        return true;
    }

    if (!WiFi.softAP(PORTAL_AP_SSID, AP_PASSWORD)) {
        ESP_LOGE(TAG, "Failed to start AP");
        return false;
    }

    // DHCP option 114 (RFC 8910): tells clients to open the portal page.
    if (!WiFi.AP.enableDhcpCaptivePortal()) {
        ESP_LOGW(TAG, "Failed to enable DHCP captive portal option");
    }

    const IPAddress ip = WiFi.softAPIP();

    if (!dns.start(DNS_PORT, "*", ip)) {  // Resolve every hostname to the device.
        ESP_LOGE(TAG, "Failed to start DNS");
        WiFi.softAPdisconnect(true);
        return false;
    }

    running = true;
    ESP_LOGI(TAG, "Started: SSID \"%s\", IP %s", PORTAL_AP_SSID, ip.toString().c_str());
    return true;
}

void portal_stop()
{
    if (!running) {
        return;
    }

    dns.stop();
    WiFi.softAPdisconnect(true);  // Disables AP mode, keeps STA.

    running = false;
    ESP_LOGI(TAG, "Stopped");
}

bool portal_is_running()
{
    return running;
}
