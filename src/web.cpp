#include "web.h"

#include <WebServer.h>
#include <WiFi.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <stdio.h>

#include "auth.h"
#include "provisioning.h"
#include "sdcard.h"
#include "settings.h"
#include "web_page.h"
#include "wifi_station.h"

#define HTTP_PORT         80
#define WEB_TASK_STACK    6144
#define WEB_TASK_PRIORITY 1
#define WEB_POLL_MS       10
#define SD_JSON_SIZE      192
#define WPA2_MIN_PASSWORD 8
#define SESSION_COOKIE    "bb32_session"

static const char *TAG = "web";

static WebServer server(HTTP_PORT);

// True if the request arrived on the setup AP (same check as the core's
// WebServer/examples/Filters).
static bool from_ap()
{
    return WiFi.AP.hasIP() && WiFi.AP.localIP() == server.client().localIP();
}

static String json_escape(const String &in)
{
    String out;
    out.reserve(in.length());

    for (const char c : in) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) >= 0x20) {
            out += c;  // Control characters are dropped.
        }
    }

    return out;
}

static void send_json(int code, const String &body)
{
    server.send(code, "application/json", body);
}

// Extracts the session token from the Cookie header into token.
static void session_token(char *token, size_t size)
{
    token[0] = '\0';

    const String cookies = server.header("Cookie");
    const String key     = SESSION_COOKIE "=";
    const int    start   = cookies.indexOf(key);

    if (start < 0) {
        return;
    }

    const int begin = start + key.length();
    int end = cookies.indexOf(';', begin);
    if (end < 0) {
        end = cookies.length();
    }

    cookies.substring(begin, end).toCharArray(token, size);
}

// Middleware: rejects requests without a live session.
static bool require_auth(WebServer &srv, Middleware::Callback next)
{
    char token[AUTH_TOKEN_SIZE];
    session_token(token, sizeof(token));

    if (auth_check(token)) {
        return next();
    }

    srv.send(401, "application/json", "{\"ok\":false,\"error\":\"unauthorized\"}");
    return true;
}

static void handle_login()
{
    char token[AUTH_TOKEN_SIZE];

    if (!auth_login(server.arg("password").c_str(), token, sizeof(token))) {
        send_json(401, "{\"ok\":false,\"error\":\"wrong password\"}");
        return;
    }

    char cookie[128];
    snprintf(cookie, sizeof(cookie),
             SESSION_COOKIE "=%s; Path=/; HttpOnly; SameSite=Strict; Max-Age=%d",
             token, AUTH_SESSION_TTL_S);

    server.sendHeader("Set-Cookie", cookie);
    send_json(200, "{\"ok\":true}");
}

static void handle_logout()
{
    char token[AUTH_TOKEN_SIZE];
    session_token(token, sizeof(token));
    auth_logout(token);

    server.sendHeader("Set-Cookie", SESSION_COOKIE "=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    send_json(200, "{\"ok\":true}");
}

static void handle_root()
{
    server.send(200, "text/html", WEB_PAGE);
}

static void handle_status()
{
    char ssid[SETTINGS_SSID_SIZE]         = "";
    char password[SETTINGS_PASSWORD_SIZE] = "";

    const bool configured =
        settings_get_wifi(ssid, sizeof(ssid), password, sizeof(password));

    String json = "{\"via_ap\":";
    json += from_ap() ? "true" : "false";
    json += ",\"online\":";
    json += provisioning_is_online() ? "true" : "false";
    json += ",\"config_mode\":";
    json += provisioning_config_mode() ? "true" : "false";
    json += ",\"wifi\":{\"configured\":";
    json += configured ? "true" : "false";
    json += ",\"ssid\":\"";
    json += json_escape(ssid);
    json += "\",\"state\":\"";
    json += wifi_station_state_name(wifi_station_get_state());
    json += "\"}}";

    send_json(200, json);
}

static void handle_sd()
{
    sdcard_info_t info;
    char json[SD_JSON_SIZE];

    sdcard_get_info(&info);

    snprintf(json, sizeof(json),
             "{\"mounted\":%s,\"type\":\"%s\",\"card_size\":%" PRIu64
             ",\"total_bytes\":%" PRIu64 ",\"used_bytes\":%" PRIu64 "}",
             info.mounted ? "true" : "false", info.type,
             info.card_size, info.total_bytes, info.used_bytes);

    send_json(200, json);
}

static void handle_scan()
{
    const int16_t result = WiFi.scanComplete();

    if (result == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);  // Async; the page polls for results.
    }

    if (result < 0) {
        send_json(200, "{\"scanning\":true}");
        return;
    }

    String json = "{\"scanning\":false,\"networks\":[";

    for (int16_t i = 0; i < result; i++) {
        if (i > 0) {
            json += ',';
        }
        json += "{\"ssid\":\"";
        json += json_escape(WiFi.SSID(i));
        json += "\",\"rssi\":";
        json += WiFi.RSSI(i);
        json += ",\"open\":";
        json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false";
        json += '}';
    }

    json += "]}";

    WiFi.scanDelete();  // Next request starts a fresh scan.
    send_json(200, json);
}

static void handle_wifi_save()
{
    const String ssid     = server.arg("ssid");
    const String password = server.arg("password");

    if (!password.isEmpty() && password.length() < WPA2_MIN_PASSWORD) {
        send_json(400, "{\"ok\":false,\"error\":\"password too short\"}");
        return;
    }

    if (!settings_set_wifi(ssid.c_str(), password.c_str())) {
        send_json(400, "{\"ok\":false,\"error\":\"invalid credentials\"}");
        return;
    }

    send_json(200, "{\"ok\":true}");
    provisioning_reconnect();  // After the reply: the STA link may drop.
}

static void handle_config_mode()
{
    provisioning_set_config_mode(server.arg("enabled") == "1");
    send_json(200, "{\"ok\":true}");
}

static void handle_not_found()
{
    // AP clients (including OS connectivity checks) land on the device page.
    if (from_ap()) {
        server.sendHeader("Location", "http://" + WiFi.AP.localIP().toString() + "/", true);
        server.send(302, "text/plain", "");
        return;
    }

    server.send(404, "text/plain", "Not found");
}

static void web_task(void *)
{
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        server.handleClient();
        xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(WEB_POLL_MS));
    }
}

bool web_start()
{
    static const char *collected_headers[] = {"Cookie"};
    server.collectHeaders(collected_headers, 1);

    // Public: the page itself (static HTML, no data) and the login endpoint.
    server.on("/", HTTP_GET, handle_root);
    server.on("/api/login", HTTP_POST, handle_login);

    // Everything else under /api requires a session.
    server.on("/api/logout", HTTP_POST, handle_logout).addMiddleware(require_auth);
    server.on("/api/status", HTTP_GET, handle_status).addMiddleware(require_auth);
    server.on("/api/sd", HTTP_GET, handle_sd).addMiddleware(require_auth);
    server.on("/api/wifi/scan", HTTP_GET, handle_scan).addMiddleware(require_auth);
    server.on("/api/wifi", HTTP_POST, handle_wifi_save).addMiddleware(require_auth);
    server.on("/api/config_mode", HTTP_POST, handle_config_mode).addMiddleware(require_auth);
    server.onNotFound(handle_not_found);

    server.begin();

    if (xTaskCreate(web_task, "web", WEB_TASK_STACK, nullptr,
                    WEB_TASK_PRIORITY, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        server.stop();
        return false;
    }

    ESP_LOGI(TAG, "Started on port %d", HTTP_PORT);
    return true;
}
