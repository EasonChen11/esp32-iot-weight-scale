#pragma once
#include "config.h"
#if AUTO_LOGGER_ENABLED
#include <Arduino.h>

/*
initAutoLogger

Initialize the automatic logging service. Schedules the initial startup
record and prepares hourly logging.

Parameters:
    none

Returns:
    void
*/
void initAutoLogger();

/*
handleAutoLogging

Call regularly from the main loop to perform scheduled logging checks.

Parameters:
    none

Returns:
    void
*/
void handleAutoLogging();

bool isStartupRecordDone();

#endif // AUTO_LOGGER_ENABLED
