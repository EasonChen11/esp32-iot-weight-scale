#include "config.h"
#if DEEP_SLEEP_ENABLED
#include "deep_sleep_manager.h"
#include "sensor_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

#if SCHEDULE_ENABLED
#include "schedule_manager.h"
#endif
#if NTP_ENABLED
#include "wifi_manager.h"
#endif

static unsigned long bootTimeMs = 0;
static bool sleepTriggered = false;

void initDeepSleep()
{
    // Release GPIO holds from previous deep sleep (all held pins are RTC GPIOs)
    gpio_hold_dis((gpio_num_t)WAKE_BTN_GND);

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
#if NTP_ENABLED
        // 未校時時提早 2 分鐘喚醒，補償 RTC 漂移並讓下次 boot 有機會重試 NTP
        if (!isTimeSynced() && seconds > 180) {
            seconds -= 120;
            Serial.println("[SLEEP] Time not synced — waking 2 min early for drift margin");
        }
#endif
        uint64_t sleepUs = (uint64_t)seconds * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleepUs);
        Serial.printf("[SLEEP] Timer wake-up set: %d seconds (next schedule)\n", seconds);
    } else {
        // No schedule or time unknown — use fallback interval to retry NTP on next boot
        uint64_t sleepUs = (uint64_t)FALLBACK_WAKEUP_SEC * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleepUs);
        Serial.printf("[SLEEP] No valid schedule — fallback wake-up in %d seconds\n", FALLBACK_WAKEUP_SEC);
    }
#else
    Serial.println("[SLEEP] No schedule module — button wake-up only");
#endif

    powerDownSensors();
    gpio_hold_en((gpio_num_t)LOADCELL1_SCK_PIN);
    gpio_hold_en((gpio_num_t)LOADCELL2_SCK_PIN);

    // Hold button GND LOW during deep sleep
    gpio_hold_en((gpio_num_t)WAKE_BTN_GND);

    // Ensure pullup on wake button persists in RTC domain
    rtc_gpio_pullup_en((gpio_num_t)WAKE_BTN_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)WAKE_BTN_PIN);

    Serial.println("[SLEEP] Entering deep sleep now...");
    Serial.flush();
    delay(100);
    esp_deep_sleep_start();
}

#endif // DEEP_SLEEP_ENABLED
