// SD card over SDMMC (1-bit bus).
#ifndef SDCARD_H
#define SDCARD_H

#include <stdint.h>

typedef struct {
    bool        mounted;
    const char  *type;        // "MMC", "SDSC", "SDHC/SDXC", "none"
    uint64_t    card_size;    // Raw card capacity, bytes
    uint64_t    total_bytes;  // FAT volume size, bytes
    uint64_t    used_bytes;   // Used space on the volume, bytes
} sdcard_info_t;

// Mounts the card at /sdcard. Call once at boot.
bool sdcard_init();

// True while a card is mounted. Cheap: safe to poll.
bool sdcard_is_mounted();

// Fills info with the current card state. Returns false if no card is mounted.
bool sdcard_get_info(sdcard_info_t *info);

#endif  // SDCARD_H
