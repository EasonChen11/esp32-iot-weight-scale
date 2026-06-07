#include "dev_mode.h"
#if DEV_MODE_ENABLED
#if GOOGLE_SHEETS_ENABLED
#include "storage/nvs_storage.h"
#endif

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
#if GOOGLE_SHEETS_ENABLED
                else if (buf.startsWith("sheets-set ")) {
                    // Format: sheets-set <url> <token>
                    String rest = buf.substring(11);
                    rest.trim();
                    int sp = rest.indexOf(' ');
                    String url = (sp > 0) ? rest.substring(0, sp) : "";
                    String token = (sp > 0) ? rest.substring(sp + 1) : "";
                    url.trim();
                    token.trim();
                    if (url.length() == 0 || token.length() == 0) {
                        Serial.println("[GSheets] usage: sheets-set <url> <token>");
                    } else {
                        saveSheetsConfig(url, token);
                    }
                }
                else if (buf == "sheets-status") {
                    String url, token;
                    if (getSheetsConfig(url, token)) {
                        // Token shown masked (length only) to keep it out of logs.
                        Serial.printf("[GSheets] NVS config: url=%s token=<%d chars>\n",
                                      url.c_str(), (int)token.length());
                    } else {
                        Serial.println("[GSheets] no NVS config — using compile-time URL/token");
                    }
                }
                else if (buf == "sheets-clear") {
                    clearSheetsConfig();
                }
#endif
                // Unknown commands silently ignored.
            }
            buf = "";
        } else if (buf.length() < 200) {
            buf += c;
        }
        // overflow chars are dropped (next newline resets the buffer)
    }
}

#endif // DEV_MODE_ENABLED
