#include "config.h"
#if OTA_ENABLED
#include "ota_manager.h"
#include <Arduino.h>

static bool otaInProgress = false;

void initOTA()
{
    Serial.printf("[OTA] Running firmware version: %s\n", FIRMWARE_VERSION);
}

// Returns true if `candidate` is a strictly newer semver than `current`.
// Parses up to three dot-separated integer fields ("1.2.0"); missing fields = 0.
// Non-numeric / malformed input compares as 0, so it never updates on garbage.
static bool isNewerVersion(const char *current, const char *candidate)
{
    int cur[3] = {0, 0, 0};
    int cand[3] = {0, 0, 0};
    sscanf(current, "%d.%d.%d", &cur[0], &cur[1], &cur[2]);
    sscanf(candidate, "%d.%d.%d", &cand[0], &cand[1], &cand[2]);
    for (int i = 0; i < 3; i++)
    {
        if (cand[i] > cur[i]) return true;
        if (cand[i] < cur[i]) return false;
    }
    return false; // equal
}

void checkOtaUpdate()
{
    Serial.printf("[OTA] selfcheck 1.1.0<1.2.0 = %d (expect 1)\n",
                  isNewerVersion("1.1.0", "1.2.0"));
    Serial.printf("[OTA] selfcheck 1.2.0<1.2.0 = %d (expect 0)\n",
                  isNewerVersion("1.2.0", "1.2.0"));
    Serial.printf("[OTA] selfcheck 1.2.0<1.1.9 = %d (expect 0)\n",
                  isNewerVersion("1.2.0", "1.1.9"));
}

bool isOtaInProgress()
{
    return otaInProgress;
}

#endif // OTA_ENABLED
