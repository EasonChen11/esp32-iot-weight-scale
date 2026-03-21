#include "config.h"
#if DEEP_SLEEP_ENABLED
#include "deep_sleep_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>

#if SCHEDULE_ENABLED
#include "schedule_manager.h"
#endif

static unsigned long bootTimeMs = 0;
static bool sleepTriggered = false;

void initDeepSleep()
{
    // Button wake-up: GPIO 32 (INPUT_PULLUP), active LOW
    pinMode(WAKE_BTN_GND, OUTPUT);
    digitalWrite(WAKE_BTN_GND, LOW);
    pinMode(WAKE_BTN_PIN, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BTN_PIN, 0);

    bootTimeMs = millis();
    sleepTriggered = false;

    // Log wake-up reason
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[SLEEP] Woke up by timer (scheduled)");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[SLEEP] Woke up by button press");
            break;
        default:
            Serial.println("[SLEEP] Woke up by power-on / reset");
            break;
    }

    Serial.printf("[SLEEP] Will stay awake for %lu ms\n", AWAKE_DURATION_MS);
}

void handleDeepSleep()
{
    if (sleepTriggered) return;
    if (millis() - bootTimeMs < AWAKE_DURATION_MS) return;

    // Awake period expired — prepare to sleep
    sleepTriggered = true;

#if SCHEDULE_ENABLED
    int seconds = getNextWakeupSeconds();
    if (seconds > 0) {
        uint64_t sleepUs = (uint64_t)seconds * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleepUs);
        Serial.printf("[SLEEP] Timer wake-up set: %d seconds (next schedule)\n", seconds);
    } else {
        Serial.println("[SLEEP] No schedule entries or time not synced — button wake-up only");
    }
#else
    Serial.println("[SLEEP] No schedule module — button wake-up only");
#endif

    Serial.println("[SLEEP] Entering deep sleep now...");
    Serial.flush();
    delay(100);
    esp_deep_sleep_start();
}

#endif // DEEP_SLEEP_ENABLED
