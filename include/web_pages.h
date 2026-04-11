#pragma once

#include <Arduino.h>

/*
Generate the complete HTML content for the main dashboard page.
This function returns the full HTML/CSS/JavaScript code for the weight monitoring
dashboard, including real-time charts, data tables, and control buttons.

Parameters:
  none

Returns:
  String: Complete HTML document as a single string

Example:
  String html = getIndexHTML();  // Returns full webpage content
*/
String getIndexHTML();

#include "config.h"
#if WIFI_CONFIG_ENABLED
String getNetworkPageHTML();
#endif
