#pragma once

// 初始化感測器 (或是模擬模式)
void initSensor();

// 在 loop() 中呼叫，負責處理 500ms 的定時採樣邏輯
void updateSensor();

// 獲取目前緩存的重量 (給網頁路由使用)
float getCachedWeight();

// 執行歸零動作
void tareSensor();

