#pragma once

#include "config.h"
#if DEV_MODE_ENABLED

#include <Arduino.h>

bool isDevMode();
void setDevMode(bool on);
void printBootModeBanner();
void handleSerialModeCommand();   // Task 3

#endif // DEV_MODE_ENABLED
