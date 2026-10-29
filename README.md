# bytebox32

[![PlatformIO](https://img.shields.io/badge/PlatformIO-pioarduino-orange?logo=platformio&logoColor=white)](https://github.com/pioarduino/platform-espressif32)
[![Chip](https://img.shields.io/badge/chip-ESP32--S3-red?logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Arduino core](https://img.shields.io/badge/Arduino--ESP32-3.3.11-00979D?logo=arduino&logoColor=white)](https://github.com/espressif/arduino-esp32)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-5.5.5-E7352C)](https://github.com/espressif/esp-idf)
[![FreeRTOS](https://img.shields.io/badge/RTOS-FreeRTOS-brightgreen)](https://www.freertos.org/)
[![Language](https://img.shields.io/badge/language-C%2B%2B-00599C?logo=cplusplus&logoColor=white)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)](LICENSE)

A small, always-on file box for your local network, built on an ESP32-S3 with an SD card.
Plug it into a wall socket, reach it from any device with a browser (Windows, macOS,
Linux, Android, iOS), and update its firmware over Wi-Fi.

> **Status:** early development. The network, web interface, authentication, OTA and
> SD card information are working. File management is next (see [Roadmap](#roadmap)).

---

## Table of contents

- [Features](#features)
- [Hardware](#hardware)
- [Status LED](#status-led)
- [Getting started](#getting-started)
- [First-time setup](#first-time-setup)
- [Web interface](#web-interface)
- [HTTP API](#http-api)
- [OTA updates](#ota-updates)
- [Project structure](#project-structure)
- [Security](#security)
- [Roadmap](#roadmap)
- [License](#license)

---

## Features

- **Captive setup portal** – the device opens its own Wi-Fi access point (`bytebox32`, WPA2)
  only when needed; phones and computers pop up the setup page automatically.
- **Runtime Wi-Fi configuration** – credentials are stored in NVS, never compiled in.
  Change networks from the web interface at any time.
- **Configuration mode on demand** – from the web settings, or by holding the **BOOT**
  button for 3 seconds when the device is unreachable.
- **Single HTTP server** for every interface (setup AP and home network), in the same
  style as WLED and Tasmota.
- **Password-protected web interface** – password-only sign-in with session cookies.
- **mDNS** – reachable as `http://bytebox32.local` on both the home network and the setup AP.
- **OTA firmware updates** with password authentication.
- **SD card** over SDMMC (1-bit bus): type, capacity and usage shown in the web interface.
- **Status RGB LED** with low, night-friendly brightness.
- **Modular FreeRTOS design** – each subsystem runs in its own task behind a small C-style API.

## Hardware

| Item | Details |
|---|---|
| Board | ESP32-S3 N16R8 (16 MB flash, 8 MB octal PSRAM), DevKitC-1 / Freenove layout |
| Storage | microSD slot wired for SDMMC 1-bit |
| Status LED | onboard WS2812 |
| Button | onboard BOOT |

### Pin map

| Function | GPIO |
|---|---|
| SD `CLK` | 39 |
| SD `CMD` | 38 |
| SD `D0` | 40 |
| WS2812 status LED | 48 |
| BOOT button | 0 |

Pins and LED levels are defined in [`include/config.h`](include/config.h).

> Some ESP32-S3 N16R8 clone boards ship with the WS2812 disconnected. If the LED never
> lights up, bridge the solder pads marked **RGB** on the board.

## Status LED

| Color | Meaning |
|---|---|
| Blue, steady | Online and SD card mounted |
| Purple, blinking | Configuration mode, or no Wi-Fi network saved yet |
| Orange, blinking | The saved Wi-Fi network keeps failing to connect |
| Red, steady | Other error (for example, SD card missing) or still starting up |

Priority when several apply: configuration → Wi-Fi → other errors → OK.

## Getting started

### Requirements

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (CLI) or the PlatformIO IDE
- A FAT32-formatted microSD card

The build uses the community-maintained
[pioarduino](https://github.com/pioarduino/platform-espressif32) platform (Arduino-ESP32 3.x),
pinned to a specific release in `platformio.ini`.

### 1. Clone

```bash
git clone https://github.com/yvrcodex/bytebox32.git
cd bytebox32
```

### 2. Create your secrets file

```bash
cp secrets.example.ini secrets.ini
```

Edit `secrets.ini` and set your own values. This file is git-ignored.

| Key | Purpose |
|---|---|
| `ap_password` | WPA2 password of the `bytebox32` setup access point (8+ characters) |
| `ota_password` | Password required to upload firmware over Wi-Fi |
| `web_password` | Password of the web interface |

Values are written without quotes. Avoid `"`, `\`, `${` and ` ;` (space followed by a semicolon).
The build fails if any password is empty.

### 3. Build and flash over USB

The first upload must be done over USB:

```bash
pio run -t upload
```

Serial monitor:

```bash
pio device monitor
```

On Linux, your user needs access to the serial port (on Arch-based systems, the `uucp` group).

## First-time setup

1. Power the board. With no saved network, the LED blinks purple and the `bytebox32`
   access point comes up.
2. Connect to `bytebox32` using your `ap_password`. The setup page opens automatically
   (otherwise browse to `http://192.168.4.1`).
3. Sign in with your `web_password`.
4. Tap **Scan networks**, pick your network, enter its password and select **Save and connect**.
5. Once connected, the access point closes and the LED turns blue.
   The device is now at `http://bytebox32.local`.

To change networks later, use **⚙ Settings → Wi-Fi network** from your home network.
If the device can no longer reach your network (new router, new password), the access
point reopens on its own, or you can hold **BOOT** for 3 seconds.

## Web interface

- **Overview** – Wi-Fi network and state, SD card type, capacity and usage.
- **⚙ Settings**
  - Wi-Fi network: scan, select, save and connect.
  - Configuration mode: keep the `bytebox32` access point open.
  - Sign out.

The same page is served on the home network and on the setup access point. Clients on the
access point land directly on the settings.

## HTTP API

All endpoints return JSON. Every endpoint except `POST /api/login` requires a valid
session cookie and answers `401` otherwise.

| Method | Path | Description |
|---|---|---|
| `POST` | `/api/login` | Body: `password`. Sets the session cookie |
| `POST` | `/api/logout` | Ends the current session |
| `GET` | `/api/status` | Wi-Fi state, saved SSID, online flag, configuration mode |
| `GET` | `/api/sd` | SD card type, capacity and usage in bytes |
| `GET` | `/api/wifi/scan` | Starts or polls an async network scan |
| `POST` | `/api/wifi` | Body: `ssid`, `password`. Saves credentials and reconnects |
| `POST` | `/api/config_mode` | Body: `enabled=1` or `enabled=0` |

Example with `curl`:

```bash
curl -c cookies.txt -d "password=YOUR_WEB_PASSWORD" http://bytebox32.local/api/login
curl -b cookies.txt http://bytebox32.local/api/sd
```

## OTA updates

After the first USB upload, firmware can be flashed over Wi-Fi:

```bash
pio run -e bytebox32_ota -t upload
```

The password comes from `ota_password` in `secrets.ini`.

During an OTA upload the ESP32 opens a TCP connection **back** to your computer on port
`3233` (set with `--host_port` in `platformio.ini`). If your firewall blocks incoming
connections, allow that port from your local network only. Example with `ufw`:

```bash
sudo ufw allow from 192.168.0.0/24 to any port 3233 proto tcp comment 'espota OTA'
```

Adjust the subnet to match your network.

> If you change `ota_password`, flash once over USB: the firmware on the board still
> expects the old password.

## Project structure

```
.
├── include/
│   ├── auth.h            Password sign-in and session tokens
│   ├── config.h          Non-secret configuration (hostname, pins, LED levels)
│   ├── discovery.h       mDNS responder
│   ├── ota.h             ArduinoOTA service
│   ├── portal.h          Setup access point and captive DNS
│   ├── provisioning.h    Station connection, retries and AP policy
│   ├── sdcard.h          SD card over SDMMC
│   ├── settings.h        Persistent settings in NVS
│   ├── status_led.h      WS2812 status LED
│   ├── web.h             HTTP server and API
│   ├── web_page.h        Web interface (HTML, CSS, JS)
│   └── wifi_station.h    Non-blocking Wi-Fi station
├── src/                  Module implementations and main.cpp
├── platformio.ini        Build and upload environments
├── secrets.example.ini   Template for secrets.ini
└── LICENSE
```

Each module is a `.h`/`.cpp` pair with a small C-style interface. `main.cpp` only starts
the modules; its `loop()` acts as a supervisor for the BOOT button and the status LED.

## Security

- **No credentials in the source tree.** Build-time secrets live in the git-ignored
  `secrets.ini`; Wi-Fi credentials are entered at runtime and stored in NVS.
- **Setup access point** uses WPA2 and only runs when needed.
- **Web interface** sessions use 128-bit tokens from the hardware RNG, delivered in
  `HttpOnly; SameSite=Strict` cookies. Passwords are compared in constant time.
- **OTA uploads** require a password.

Known limitations:

- The web interface uses plain HTTP. The password travels unencrypted on your local
  network at sign-in. Do not expose the device to the internet.
- Sessions are kept in RAM; a reboot signs everyone out.
- Firmware secrets can be read from a flash dump unless flash encryption is enabled.

## Roadmap

- [x] Wi-Fi provisioning with captive portal
- [x] Single HTTP server for AP and home network
- [x] OTA updates
- [x] SD card information
- [x] Status LED
- [x] Password-protected web interface
- [ ] File manager: browse, upload, download, delete
- [ ] `esp-save` command-line tool to copy project folders to the device
- [ ] Change the web password from the settings (hash stored in NVS)
- [ ] Brute-force protection on sign-in
- [ ] WebDAV access
- [ ] Companion webview app

## License

Released under the [MIT License](LICENSE).
