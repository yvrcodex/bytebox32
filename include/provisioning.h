// Network provisioning: station connection, retries and setup AP policy.
//
// The setup AP is up only when needed:
//   - no saved network
//   - the saved network failed to connect (retried every 3 s in background)
//   - configuration mode was requested (web settings or BOOT button)
#ifndef PROVISIONING_H
#define PROVISIONING_H

typedef void (*provisioning_cb_t)();

// Starts the provisioning task. on_online runs in that task every time
// the station connects.
bool provisioning_start(provisioning_cb_t on_online);

// Reconnects using the credentials currently stored in settings.
void provisioning_reconnect();

// Forces the setup AP on (true) or returns to the automatic policy (false).
void provisioning_set_config_mode(bool enabled);

bool provisioning_config_mode();

// True while the station is connected to the saved network.
bool provisioning_is_online();

// True while the saved network keeps failing to connect.
bool provisioning_has_wifi_error();

#endif  // PROVISIONING_H
