#pragma once

/**
 * @brief 初始化感測器
 * @param savedOffset 從外部傳入的絕對零點數值。若為 0 則執行自動歸零。
 */
void initSensor(long savedOffset = 0);

/**
 * @brief 在 loop() 中定時更新重量快取 (500ms)
 */
void updateSensor();

/**
 * @brief 取得目前快取的重量 (kg)
 */
float getCachedWeight();

/**
 * @brief 執行一般的「去皮/歸零」 (僅存於記憶體)
 */
void tareSensor();

/**
 * @brief 擷取並回傳當前的絕對零點原始數值
 * @return long 原始的 Offset 數值，供外部模組存檔使用
 */
long captureAbsoluteOffset();