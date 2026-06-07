#pragma once
#include "config.h"
#if OTA_ENABLED

/*
 * ota_manager — HTTPS pull OTA from GitHub Releases.
 *
 * Flow (called once per boot/wake from the Core 0 task, after WiFi is up):
 *   fetch manifest.json -> compare version -> download firmware.bin
 *   -> verify sha256 -> write to inactive OTA slot -> reboot.
 *
 * Enable with:  #define OTA_ENABLED true  in config.h
 */

void initOTA();          // call once in setup(); validates the running image (rollback)
void checkOtaUpdate();    // call once after WiFi connects; performs the whole flow
bool isOtaInProgress();   // true while downloading/applying — deep-sleep must wait

#endif // OTA_ENABLED
