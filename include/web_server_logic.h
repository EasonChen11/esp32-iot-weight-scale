#pragma once
#include "config.h"
#if WEB_SERVER_ENABLED
#include <WebServer.h>

/*
Initialize and register all HTTP routes for the web server.
This function sets up all REST API endpoints including data endpoints
(weight, records), control endpoints (tare, calibration), and serves
the main web interface.

Parameters:
  server (WebServer&): Reference to the ESP32 WebServer instance

Returns:
  void

Example:
  WebServer server(80);
  initWebRoutes(server);     (sets up all routes and starts the server)
*/
void initWebRoutes(WebServer &server);

#endif // WEB_SERVER_ENABLED