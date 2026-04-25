/**
   * ESP32 Weight Scale — Google Sheets Receiver                                                                        
   *                                                                                                                    
   * 接收 ESP32 POST 的重量記錄（批次），寫入試算表。
   * 回傳已成功寫入的 record ID 列表，讓 ESP32 標記為已同步。                                                           
   */                                                                                                                   
                                                                                                                        
  // ---- 設定 ----                                                                                                     
  const SHEET_NAME = "WeightData";  // 試算表分頁名稱
                                                                                                                        
  function doPost(e) {
    try {                                                                                                               
      const payload = JSON.parse(e.postData.contents);
      const records = payload.records; // 陣列                                                                          
   
      if (!records || !Array.isArray(records) || records.length === 0) {                                                
        return jsonResponse({ success: false, error: "no records" });
      }                                                                                                                 
                  
      const sheet = getOrCreateSheet();                                                                                 
      const existingIds = getExistingIds(sheet);
      const receivedIds = [];                                                                                           
                                                                                                                        
      for (const r of records) {
        // 去重：如果 ID 已存在就跳過寫入，但仍回傳 ack                                                                 
        if (existingIds.has(String(r.id))) {                                                                            
          receivedIds.push(r.id);
          continue;                                                                                                     
        }         
                                                                                                                        
        // 寫入新列：ID | Date | Time | Sensor1 | Sensor2 | Total(公式)                                                 
        const newRow = sheet.getLastRow() + 1;
        sheet.getRange(newRow, 1, 1, 5).setValues([[                                                                    
          r.id,   
          r.date,                                                                                                       
          r.time, 
          r.sensor1,                                                                                                    
          r.sensor2
        ]]);
        // F 欄 = Total，用公式自動計算 D+E
        sheet.getRange(newRow, 6).setFormula("=D" + newRow + "+E" + newRow);                                            
   
        existingIds.add(String(r.id));                                                                                  
        receivedIds.push(r.id);
      }                                                                                                                 
      // ★ 新增：按 ID 欄（A欄）升序排列，保持表頭不動                                                                  
      const lastRow = sheet.getLastRow();
      if (lastRow > 1) {                                                                                                
        sheet.getRange(2, 1, lastRow - 1, 6).sort({column: 1, ascending: true});                                        
      } 
      return jsonResponse({
        success: true,
        received_ids: receivedIds,
        count: receivedIds.length
      });                                                                                                               
   
    } catch (err) {                                                                                                     
      return jsonResponse({ success: false, error: err.message });
    }
  }

  /** 取得或建立目標分頁，含表頭 */                                                                                     
  function getOrCreateSheet() {
    const ss = SpreadsheetApp.getActiveSpreadsheet();                                                                   
    let sheet = ss.getSheetByName(SHEET_NAME);                                                                          
   
    if (!sheet) {                                                                                                       
      sheet = ss.insertSheet(SHEET_NAME);
      sheet.getRange(1, 1, 1, 6).setValues([[
        "ID", "Date", "Time", "Sensor1 (kg)", "Sensor2 (kg)", "Total (kg)"                                              
      ]]);
      sheet.getRange(1, 1, 1, 6).setFontWeight("bold");                                                                 
    }                                                                                                                   
   
    return sheet;                                                                                                       
  }               

  /** 讀取 A 欄所有已存在的 ID，回傳 Set */                                                                             
  function getExistingIds(sheet) {
    const lastRow = sheet.getLastRow();                                                                                 
    const ids = new Set();
                                                                                                                        
    if (lastRow <= 1) return ids; // 只有表頭
                                                                                                                        
    const values = sheet.getRange(2, 1, lastRow - 1, 1).getValues();                                                    
    for (const row of values) {
      ids.add(String(row[0]));                                                                                          
    }             

    return ids;
  }

  /** 回傳 JSON response */                                                                                             
  function jsonResponse(data) {
    return ContentService                                                                                               
      .createTextOutput(JSON.stringify(data))
      .setMimeType(ContentService.MimeType.JSON);
  }                                                                                                                     
   
  // ---- 測試用 ----                                                                                                   
  function testDoPost() {
    const mockEvent = {
      postData: {
        contents: JSON.stringify({
          records: [                                                                                                    
            { id: 1, date: "2026-03-30", time: "14:30:00", sensor1: 25.3, sensor2: 22.1 },
            { id: 2, date: "2026-03-30", time: "14:35:00", sensor1: 25.5, sensor2: 22.0 }                               
          ]                                                                                                             
        })                                                                                                              
      }                                                                                                                 
    };            
    const result = doPost(mockEvent);
    Logger.log(result.getContent());
  }                                 