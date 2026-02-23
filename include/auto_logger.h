#pragma once

#include <Arduino.h>

/**
 * 初始化自動紀錄服務
 */
void initAutoLogger();

/**
 * 在 loop 中持續呼叫，處理定時檢查
 */
void handleAutoLogging();
