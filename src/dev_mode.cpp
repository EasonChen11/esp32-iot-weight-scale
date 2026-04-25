#include "dev_mode.h"
#if DEV_MODE_ENABLED

static bool g_devMode = false;

bool isDevMode() { return g_devMode; }

void setDevMode(bool on)
{
    if (g_devMode == on) return;
    g_devMode = on;
    Serial.printf("[Mode] %s\n", on ? "DEVELOPER ON" : "USER");
}

void printBootModeBanner()
{
    Serial.printf("[Mode] Current: %s  (type \"dev-on\" to enable developer mode)\n",
                  g_devMode ? "DEVELOPER" : "USER");
}

void handleSerialModeCommand() { /* Task 3 fills this in */ }

#endif // DEV_MODE_ENABLED
