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

void handleSerialModeCommand()
{
    static String buf;
    while (Serial.available()) {
        char c = (char)Serial.read();
        // Accept LF, CR, or CRLF as line terminator. Empty buffer between
        // CR and LF is harmless (filtered by buf.length() check below).
        if (c == '\r' || c == '\n') {
            buf.trim();
            if (buf.length() > 0) {
                if      (buf == "dev-on")     setDevMode(true);
                else if (buf == "dev-off")    setDevMode(false);
                else if (buf == "dev-status") {
                    Serial.printf("[Mode] Current: %s\n",
                                  isDevMode() ? "DEVELOPER" : "USER");
                }
                // Unknown commands silently ignored.
            }
            buf = "";
        } else if (buf.length() < 32) {
            buf += c;
        }
        // overflow chars are dropped (next newline resets the buffer)
    }
}

#endif // DEV_MODE_ENABLED
