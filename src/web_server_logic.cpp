#include "web_server_logic.h"
#include "web_pages.h"       // 為了 getIndexHTML
#include "sensor_manager.h"  // 為了 getWeight 和 tareSensor
#include "storage_manager.h" // 為了儲存紀錄相關函式
#include <time.h>            // 為了時間同步

void initWebRoutes(WebServer &server)
{
    // 使用 [&] 代表擷取外部作用域的所有引用，這樣才能在裡面使用 server

    /* data routes */
    // 根目錄回傳主頁面
    server.on("/", [&server]()
              { server.send(200, "text/html", getIndexHTML()); });
    // 獲取即時重量數據
    server.on("/data", [&server]()
              { 
        // 建議這裡呼叫 getWeight() 確保拿到的數據是最新的
        server.send(200, "text/plain", String(getCachedWeight(), 2)); });
    // 執行歸零動作
    server.on("/tare", [&server]()
              {
        tareSensor();
        server.send(200, "text/plain", "Tared Successfully"); });

    /* control routes */
    // 時間同步路由
    server.on("/sync", [&server]()
              {
        if (server.hasArg("t")) {
            // Unix Timestamp 在 2026 年約為 1.7 億，toInt() (long) 尚可應付
            // 但使用 atoll 處理 long long 會更安全
            time_t t = (time_t)server.arg("t").substring(0, 10).toInt(); 
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, NULL);
            
            Serial.print("時間同步成功: ");
            Serial.println(t);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Missing time argument");
        } });
    // 新增一個獲取目前時間的路由給網頁抓取
    server.on("/time", [&server]()
              {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        server.send(200, "text/plain", "Not Synced");
        return;
    }
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    server.send(200, "text/plain", String(buf)); });

    // 獲取所有儲存的紀錄
    server.on("/get-records", [&server]()
              { server.send(200, "application/json", getRecordsJson()); });

    // 新增紀錄並回傳更新後的列表
    server.on("/add-record", [&server]()
              {
    String t = server.arg("t");
    String w = server.arg("w");
    addRecordToStorage(t, w);
    server.send(200, "application/json", getRecordsJson()); });

    // 刪除紀錄並回傳更新後的列表
    server.on("/del-record", [&server]()
              {
    int index = server.arg("i").toInt();
    deleteRecordFromStorage(index);
    server.send(200, "application/json", getRecordsJson()); });
    // 清除所有紀錄
    server.on("/clear-records", [&server]()
              {
                  clearRecordsInStorage();
                  server.send(200, "application/json", "[]"); // 回傳空陣列
              });

    server.on("/set-zero", [&server]()
              {    
    long newOffset = captureAbsoluteOffset();// 1. 叫 Sensor 擷取當前的 Offset 原始值
    saveAbsoluteOffset(newOffset); // 2. 叫 Storage 把這個值存進 LittleFS
    Serial.printf("[Web] 已完成絕對零點校正，新 Offset: %ld\n", newOffset);
    server.send(200, "text/plain", "OK"); });// 3. 回傳簡單的字串，前端的 fetch 會接收這個 data
    server.begin();
}