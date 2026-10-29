#include "ota.h"

#include <ArduinoOTA.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"

#define OTA_TASK_STACK_SIZE 4096
#define OTA_TASK_PRIORITY   1
#define OTA_POLL_PERIOD_MS  20


static_assert(sizeof(OTA_PASSWORD) > 1, "OTA_PASSWORD is empty: set ota_password in secrets.ini");
static const char *TAG = "ota";

static void ota_task(void *)
{
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        ArduinoOTA.handle();
        xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(OTA_POLL_PERIOD_MS));
    }
}

bool ota_init()
{
    ArduinoOTA.setHostname(DEVICE_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setMdnsEnabled(false);  // mDNS is owned by the discovery module.

    ArduinoOTA
        .onStart([]() { ESP_LOGI(TAG, "Update started"); })
        .onEnd([]() { ESP_LOGI(TAG, "Update finished, rebooting"); })
        .onError([](ota_error_t error) {
            ESP_LOGE(TAG, "Update failed, error %u", static_cast<unsigned>(error));
        });

    ArduinoOTA.begin();

    if (xTaskCreate(ota_task, "ota", OTA_TASK_STACK_SIZE, nullptr,
                    OTA_TASK_PRIORITY, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return false;
    }

    ESP_LOGI(TAG, "Ready at %s.local", DEVICE_HOSTNAME);
    return true;
}
