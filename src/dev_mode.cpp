#include "dev_mode.h"
#if DEV_MODE_ENABLED
#include "storage/nvs_storage.h"

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
                else if (buf.startsWith("ap-set ")) {
                    // Format: ap-set <ssid> <pass>  (password may not contain spaces)
                    String rest = buf.substring(7);
                    rest.trim();
                    int sp = rest.indexOf(' ');
                    String ssid = (sp > 0) ? rest.substring(0, sp) : "";
                    String pass = (sp > 0) ? rest.substring(sp + 1) : "";
                    ssid.trim();
                    pass.trim();
                    if (ssid.length() == 0 || ssid.length() > 32 ||
                        pass.length() < 8 || pass.length() > 63 || pass.indexOf(' ') >= 0) {
                        Serial.println("[WiFi] usage: ap-set <ssid> <pass>  (ssid 1-32 chars, pass 8-63 chars, no spaces)");
                    } else {
                        saveApConfig(ssid, pass);
                        Serial.println("[WiFi] AP config saved — reboot to apply");
                    }
                }
                else if (buf == "ap-status") {
                    String ssid, pass;
                    if (getApConfig(ssid, pass)) {
                        Serial.printf("[WiFi] AP config (source: NVS): SSID='%s' pass=<%d chars>\n",
                                      ssid.c_str(), (int)pass.length());
                    } else {
                        Serial.println("[WiFi] AP config: none — using compile-time AP SSID/pass");
                    }
                }
                else if (buf == "ap-clear") {
                    clearApConfig();
                    Serial.println("[WiFi] AP config cleared — reboot to apply");
                }
                else if (buf.startsWith("mqtt-set ")) {
                    // Format: mqtt-set <ip> <port>
                    String rest = buf.substring(9);
                    rest.trim();
                    int sp = rest.indexOf(' ');
                    String ip = (sp > 0) ? rest.substring(0, sp) : "";
                    String portStr = (sp > 0) ? rest.substring(sp + 1) : "";
                    ip.trim();
                    portStr.trim();
                    long port = portStr.toInt();
                    if (ip.length() == 0 || port < 1 || port > 65535) {
                        Serial.println("[MQTT] usage: mqtt-set <ip> <port>  (port 1-65535)");
                    } else {
                        saveMqttConfig(ip, (uint16_t)port);
                        Serial.println("[MQTT] broker config saved — reboot to apply");
                    }
                }
                else if (buf == "mqtt-status") {
                    String ip;
                    uint16_t port;
                    if (getMqttConfig(ip, port)) {
                        Serial.printf("[MQTT] broker config (source: NVS): %s:%u\n", ip.c_str(), port);
                    } else {
                        Serial.println("[MQTT] broker config: none — using compile-time broker");
                    }
                }
                else if (buf == "mqtt-clear") {
                    clearMqttConfig();
                    Serial.println("[MQTT] broker config cleared — reboot to apply");
                }
                // Unknown commands silently ignored.
            }
            buf = "";
        } else if (buf.length() < 300) {
            buf += c;
        }
        // overflow chars are dropped (next newline resets the buffer)
    }
}

#endif // DEV_MODE_ENABLED
