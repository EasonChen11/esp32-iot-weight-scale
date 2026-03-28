#include "config.h"
#if OLED_ENABLED
#include "oled_manager.h"
#include "sensor_manager.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  32

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum DisplayMode { MODE_TOTAL = 0, MODE_SENSOR1, MODE_SENSOR2, MODE_COUNT };
static DisplayMode currentMode = MODE_TOTAL;

// Auto-cycle state
static unsigned long modeStartMs = 0;

void initOLED()
{
    // Power the OLED from a GPIO pin — avoids using the shared 3.3V/5V rails
    pinMode(OLED_PWR_PIN, OUTPUT);
    digitalWrite(OLED_PWR_PIN, HIGH);
    delay(10); // let rail stabilise before I2C init

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(400000); // Fast-mode: cuts I2C transfer ~90 ms → ~23 ms

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("[OLED] SSD1306 not found — check wiring");
        return;
    }
    display.clearDisplay();
    display.display();
    Serial.println("[OLED] Initialized");
}

// Layout for 128×32:
//   Y= 0  — mode label  (textSize 1, 8 px tall)
//   Y=10  — weight value (textSize 2, 16 px tall) + "kg" unit
// Total height used: 10 + 16 = 26 px (fits within 32 px)
static void drawWeight(const char *label, float kg)
{
    display.clearDisplay();

    // Mode label — small text at top
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(label);

    // Weight value — medium
    display.setTextSize(2);
    display.setCursor(0, 14);
    char buf[12];
    snprintf(buf, sizeof(buf), "%.2f", kg);
    display.print(buf);

    // Unit — right-aligned, same row as weight
    display.setTextSize(1);
    display.setCursor(104, 22);
    display.print("kg");

    display.display();
}

void handleOLED()
{
    unsigned long now = millis();

    // Auto-cycle: Total stays longer, each sensor view is shorter
    unsigned long showDuration = (currentMode == MODE_TOTAL) ? OLED_TOTAL_SHOW_MS : OLED_SENSOR_SHOW_MS;
    if (now - modeStartMs >= showDuration)
    {
        currentMode = (DisplayMode)((currentMode + 1) % MODE_COUNT);
        modeStartMs = now;
    }

    // Refresh display at ~5 Hz
    static unsigned long lastRefreshMs = 0;
    if (now - lastRefreshMs < 200) return;
    lastRefreshMs = now;

    switch (currentMode)
    {
    case MODE_TOTAL:   drawWeight("Total",    getCachedWeight());  break;
    case MODE_SENSOR1: drawWeight("Sensor 1", getCachedWeight1()); break;
    case MODE_SENSOR2: drawWeight("Sensor 2", getCachedWeight2()); break;
    default: break;
    }
}

#endif // OLED_ENABLED
