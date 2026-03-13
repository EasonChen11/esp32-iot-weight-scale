#pragma once
#include "config.h"
#if DEEP_SLEEP_ENABLED

/*
 * deep_sleep_manager — ESP32 deep-sleep + button wake-up feature
 *
 * Planned behaviour:
 *   - ESP32 enters deep sleep after an idle timeout to save power.
 *   - A push button on WAKE_BTN_PIN (GPIO 32) triggers ext0 wake-up.
 *   - On wake, the OLED is re-initialised and the display resumes.
 *
 * Enable with:  #define DEEP_SLEEP_ENABLED true  in config.h
 * Pins used:    WAKE_BTN_PIN (GPIO 32, INPUT_PULLUP, active LOW — supports ext0 wake)
 *               WAKE_BTN_GND (GPIO 33, OUTPUT LOW — acts as GND; button draws only µA)
 */

void initDeepSleep();
void handleDeepSleep();   // call from loop; triggers sleep after idle timeout

#endif // DEEP_SLEEP_ENABLED
