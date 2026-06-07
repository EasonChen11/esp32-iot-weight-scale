#include "config.h"
#if OTA_ENABLED
#include "ota_manager.h"
#include <Arduino.h>

static bool otaInProgress = false;

void initOTA()
{
    Serial.printf("[OTA] Running firmware version: %s\n", FIRMWARE_VERSION);
}

void checkOtaUpdate()
{
    // Implemented across Tasks 4-7.
}

bool isOtaInProgress()
{
    return otaInProgress;
}

#endif // OTA_ENABLED
