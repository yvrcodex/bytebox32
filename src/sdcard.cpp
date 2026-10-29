#include "sdcard.h"

#include <SD_MMC.h>
#include <esp_log.h>
#include <inttypes.h>

#include "config.h"

#define SD_MOUNT_POINT "/sdcard"
#define SD_MODE_1BIT   true
#define BYTES_PER_MB   (1024ULL * 1024ULL)

static const char *TAG = "sdcard";

static const char *type_name(sdcard_type_t type)
{
    switch (type) {
    case CARD_MMC:  return "MMC";
    case CARD_SD:   return "SDSC";
    case CARD_SDHC: return "SDHC/SDXC";  // The driver reports both as SDHC.
    case CARD_NONE: return "none";
    default:        return "unknown";
    }
}

bool sdcard_init()
{
    if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
        ESP_LOGE(TAG, "Failed to set pins");
        return false;
    }

    if (!SD_MMC.begin(SD_MOUNT_POINT, SD_MODE_1BIT)) {
        ESP_LOGE(TAG, "Mount failed (no card or unsupported format)");
        return false;
    }

    ESP_LOGI(TAG, "Mounted at %s: %s, %" PRIu64 " MB",
             SD_MOUNT_POINT, type_name(SD_MMC.cardType()),
             SD_MMC.cardSize() / BYTES_PER_MB);
    return true;
}

bool sdcard_is_mounted()
{
    return SD_MMC.cardType() != CARD_NONE;
}

bool sdcard_get_info(sdcard_info_t *info)
{
    if (info == nullptr) {
        return false;
    }

    const sdcard_type_t type = SD_MMC.cardType();

    info->mounted     = type != CARD_NONE;
    info->type        = type_name(type);
    info->card_size   = info->mounted ? SD_MMC.cardSize() : 0;
    info->total_bytes = info->mounted ? SD_MMC.totalBytes() : 0;
    info->used_bytes  = info->mounted ? SD_MMC.usedBytes() : 0;

    return info->mounted;
}
