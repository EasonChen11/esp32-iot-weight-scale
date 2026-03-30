#pragma once
#include "config.h"
#if GOOGLE_SHEETS_ENABLED

void initGoogleSheets();
void handleGoogleSheetsSync();
String triggerGoogleSheetsSync();

#endif // GOOGLE_SHEETS_ENABLED
