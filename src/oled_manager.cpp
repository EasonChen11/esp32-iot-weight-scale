#include "config.h"
#if OLED_ENABLED
#include "oled_manager.h"
#include "sensor_manager.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

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

// Layout for 128×64:
//   Y= 0  — mode label  (textSize 2, 16 px tall) ends at Y=16
//   Y=22  — weight value (textSize 3, 24 px tall) ends at Y=46
//   Y=30  — "kg" unit   (textSize 2, 16 px tall) ends at Y=46  (same baseline as number, X=96)
static void drawWeight(const char *label, float kg)
{
    display.clearDisplay();

    // Mode label — larger text so it is clearly visible
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(label);

    // Weight value — large
    display.setTextSize(3);
    display.setCursor(0, 22);
    char buf[12];
    snprintf(buf, sizeof(buf), "%.2f", kg);
    display.print(buf);

    // Unit — same baseline as number (both end at Y=46)
    display.setTextSize(2);
    display.setCursor(96, 30);
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
