#pragma once
#include "config.h"
#if OLED_ENABLED

/*
initOLED

Initialize the SSD1306 OLED display and the mode-select button.
Uses custom I2C pins (OLED_SDA_PIN / OLED_SCL_PIN) defined in config.h
to avoid conflict with the HX711 modules on GPIO 21/22.

Parameters:
  none

Returns:
  void
*/
void initOLED();

/*
handleOLED

Update the OLED display and poll the mode button.
Call regularly from the Core 0 task loop.

Button press (OLED_BUTTON_PIN) cycles the display mode:
  Total → Sensor 1 → Sensor 2 → Total → ...

Parameters:
  none

Returns:
  void
*/
void handleOLED();

#endif // OLED_ENABLED
