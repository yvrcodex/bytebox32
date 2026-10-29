#include "auth.h"

#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AUTH_MAX_SESSIONS  4    // Concurrent devices (phone, PC, app...)
#define AUTH_TOKEN_BYTES   16

static_assert(sizeof(WEB_PASSWORD) > 1, "WEB_PASSWORD is empty: set web_password in secrets.ini");
static_assert(AUTH_TOKEN_SIZE == AUTH_TOKEN_BYTES * 2 + 1, "Token size mismatch");

static const char *TAG = "auth";

typedef struct {
    char    token[AUTH_TOKEN_SIZE];
    int64_t expires_us;  // 0 = free slot
} session_t;

static session_t sessions[AUTH_MAX_SESSIONS];

static int64_t now_us()
{
    return esp_timer_get_time();
}

// Compares in time independent of where the strings differ.
static bool secure_equals(const char *a, const char *b)
{
    const size_t len_a = strlen(a);
    const size_t len_b = strlen(b);
    const size_t len   = len_a > len_b ? len_a : len_b;

    uint8_t diff = static_cast<uint8_t>(len_a != len_b);

    for (size_t i = 0; i < len; i++) {
        const uint8_t ca = i < len_a ? static_cast<uint8_t>(a[i]) : 0;
        const uint8_t cb = i < len_b ? static_cast<uint8_t>(b[i]) : 0;
        diff |= ca ^ cb;
    }

    return diff == 0;
}

static bool session_alive(const session_t *s)
{
    return s->expires_us != 0 && s->expires_us > now_us();
}

static session_t *find_session(const char *token)
{
    if (token == nullptr || strlen(token) != AUTH_TOKEN_SIZE - 1) {
        return nullptr;
    }

    for (session_t &s : sessions) {
        if (session_alive(&s) && secure_equals(s.token, token)) {
            return &s;
        }
    }

    return nullptr;
}

// Free or expired slot first, otherwise the session closest to expiry.
static session_t *claim_slot()
{
    session_t *oldest = &sessions[0];

    for (session_t &s : sessions) {
        if (!session_alive(&s)) {
            return &s;
        }
        if (s.expires_us < oldest->expires_us) {
            oldest = &s;
        }
    }

    return oldest;
}

bool auth_login(const char *password, char *token, size_t token_size)
{
    if (password == nullptr || token == nullptr || token_size < AUTH_TOKEN_SIZE) {
        return false;
    }

    if (!secure_equals(password, WEB_PASSWORD)) {
        ESP_LOGW(TAG, "Login failed");
        return false;
    }

    uint8_t raw[AUTH_TOKEN_BYTES];
    esp_fill_random(raw, sizeof(raw));  // Hardware RNG

    session_t *s = claim_slot();

    for (size_t i = 0; i < sizeof(raw); i++) {
        snprintf(&s->token[i * 2], 3, "%02x", raw[i]);
    }
    s->expires_us = now_us() + static_cast<int64_t>(AUTH_SESSION_TTL_S) * 1000000;

    memcpy(token, s->token, AUTH_TOKEN_SIZE);

    ESP_LOGI(TAG, "Login succeeded");
    return true;
}

bool auth_check(const char *token)
{
    return find_session(token) != nullptr;
}

void auth_logout(const char *token)
{
    session_t *s = find_session(token);

    if (s != nullptr) {
        memset(s, 0, sizeof(*s));
    }
}
