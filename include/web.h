// Single HTTP server for every interface (setup AP and home network).
#ifndef WEB_H
#define WEB_H

// Registers all routes and starts the server task. Call once at boot.
bool web_start();

#endif  // WEB_H
