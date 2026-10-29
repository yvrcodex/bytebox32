// Password-only web authentication with in-RAM session tokens.
// Not thread-safe: call only from the web server task.
#ifndef AUTH_H
#define AUTH_H

#include <stddef.h>

#define AUTH_TOKEN_SIZE    33                 // 16 random bytes as hex + NUL
#define AUTH_SESSION_TTL_S (7 * 24 * 3600)  // Session lifetime, seconds

// Checks the password and, on success, writes a new session token.
bool auth_login(const char *password, char *token, size_t token_size);

// True if token belongs to a live session.
bool auth_check(const char *token);

// Ends the session that owns token.
void auth_logout(const char *token);

#endif  // AUTH_H
