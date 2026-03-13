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

// Per-button debounce state
static bool         lastNextState  = HIGH;
static bool         lastPrevState  = HIGH;
static unsigned long lastNextMs    = 0;
static unsigned long lastPrevMs    = 0;

void initOLED()
{
    // Power the OLED from a GPIO pin — avoids using the shared 3.3V/5V rails
    pinMode(OLED_PWR_PIN, OUTPUT);
    digitalWrite(OLED_PWR_PIN, HIGH);
    delay(10); // let rail stabilise before I2C init

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    // GPIO 32 has internal pull-up; GPIO 35 is input-only (external pull-up required)
    pinMode(OLED_BTN_NEXT_PIN, INPUT_PULLUP);
    pinMode(OLED_BTN_PREV_PIN, INPUT);

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
//   Y= 4  — mode label  (textSize 1, 8 px tall)
//   Y=20  — weight value (textSize 3, 24 px tall) ends at Y=44
//   Y=50  — "kg" unit   (textSize 2, 16 px tall) ends at Y=66 (clipped to 64)
static void drawWeight(const char *label, float kg)
{
    display.clearDisplay();

    // Mode label
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 4);
    display.print(label);

    // Weight value — large
    display.setTextSize(3);
    display.setCursor(0, 20);
    char buf[12];
    snprintf(buf, sizeof(buf), "%.2f", kg);
    display.print(buf);

    // Unit
    display.setTextSize(2);
    display.setCursor(0, 48);
    display.print("kg");

    display.display();
}

void handleOLED()
{
    unsigned long now = millis();

    // Next button (GPIO 32, INPUT_PULLUP, active LOW)
    bool nextBtn = digitalRead(OLED_BTN_NEXT_PIN);
    if (nextBtn == LOW && lastNextState == HIGH && (now - lastNextMs) > 50)
    {
        currentMode = (DisplayMode)((currentMode + 1) % MODE_COUNT);
        lastNextMs  = now;
    }
    lastNextState = nextBtn;

    // Prev button (GPIO 35, external pull-up, active LOW)
    bool prevBtn = digitalRead(OLED_BTN_PREV_PIN);
    if (prevBtn == LOW && lastPrevState == HIGH && (now - lastPrevMs) > 50)
    {
        currentMode = (DisplayMode)((currentMode + MODE_COUNT - 1) % MODE_COUNT);
        lastPrevMs  = now;
    }
    lastPrevState = prevBtn;

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
