#include "config.h"
#if DEEP_SLEEP_ENABLED
#include "deep_sleep_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>

// TODO: define idle timeout in config.h, e.g.:
//   const unsigned long DEEP_SLEEP_IDLE_MS = 60000; // 1 min

static unsigned long lastActivityMs = 0;

void initDeepSleep()
{
    // Wake-up button: GPIO 32 (INPUT_PULLUP) → ext0 wake on LOW
    pinMode(WAKE_BTN_GND, OUTPUT);
    digitalWrite(WAKE_BTN_GND, LOW);
    pinMode(WAKE_BTN_PIN, INPUT_PULLUP);

    // Configure ext0 wake-up: wake when WAKE_BTN_PIN goes LOW
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BTN_PIN, 0);

    lastActivityMs = millis();
    Serial.println("[SLEEP] Deep-sleep manager initialized");
}

void handleDeepSleep()
{
    // TODO: reset lastActivityMs on sensor update or button press
    // TODO: enter deep sleep after idle timeout:
    //   if (millis() - lastActivityMs >= DEEP_SLEEP_IDLE_MS) {
    //       Serial.println("[SLEEP] Entering deep sleep");
    //       esp_deep_sleep_start();
    //   }
}

#endif // DEEP_SLEEP_ENABLED
