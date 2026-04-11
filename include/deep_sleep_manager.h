#pragma once
#include "config.h"
#if DEEP_SLEEP_ENABLED

/*
 * deep_sleep_manager — ESP32 scheduled deep-sleep + button wake-up
 *
 * Behaviour:
 *   - On boot, ESP32 stays awake for AWAKE_DURATION_MS (default 10 min).
 *   - After the awake period, calculates next scheduled wake time from
 *     the schedule_manager and sets a timer wake-up.
 *   - A push button on WAKE_BTN_PIN (GPIO 32) triggers ext0 wake-up at any time.
 *
 * Enable with:  #define DEEP_SLEEP_ENABLED true  in config.h
 */

void initDeepSleep();
void handleDeepSleep();   // call from Core 0 loop; enters sleep after awake period
void handleWakeButton();  // call from Core 0 loop; resets awake countdown on button press

#endif // DEEP_SLEEP_ENABLED
