// mDNS discovery: answers DEVICE_HOSTNAME.local on every interface (AP and STA).
#ifndef DISCOVERY_H
#define DISCOVERY_H

// Starts the mDNS responder. Call once at boot, after wifi_station_init().
bool discovery_init();

#endif  // DISCOVERY_H
