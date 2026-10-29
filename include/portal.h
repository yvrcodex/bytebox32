// Setup access point: soft AP + captive DNS. Pages are served by web.
#ifndef PORTAL_H
#define PORTAL_H

// Brings up the soft AP and the captive DNS.
bool portal_start();

// Tears down the captive DNS and the soft AP.
void portal_stop();

bool portal_is_running();

#endif  // PORTAL_H
