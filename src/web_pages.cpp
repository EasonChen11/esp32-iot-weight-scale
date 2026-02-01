#include "web_pages.h"

String getIndexHTML()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <meta charset='UTF-8'>
    <title>ESP32 智能秤監控系統</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f4; padding: 20px; margin: 0; }
        .card { background: white; padding: 20px; border-radius: 20px; display: inline-block; box-shadow: 0 4px 15px rgba(0,0,0,0.1); width: 95%; max-width: 500px; }
        h1 { color: #333; font-size: 22px; margin-bottom: 5px; }
        #weight { font-size: 56px; color: #e67e22; font-weight: bold; margin: 10px 0; }
        .unit { color: #7f8c8d; font-size: 18px; margin-bottom: 10px; }
        .time-info { color: #95a5a6; font-size: 13px; margin-bottom: 15px; }
        
        .chart-container { height: 180px; width: 100%; margin-bottom: 20px; }

        /* 按鈕樣式 */
        .btn-group { display: flex; gap: 10px; justify-content: center; margin-bottom: 25px; }
        button { border: none; color: white; padding: 12px 24px; font-size: 16px; border-radius: 8px; cursor: pointer; transition: all 0.2s; box-shadow: 0 4px rgba(0,0,0,0.1); outline: none; }
        button:active { transform: translateY(2px); box-shadow: 0 2px rgba(0,0,0,0.1); }
        
        .btn-tare { background-color: #e74c3c; } 
        .btn-record { background-color: #2ecc71; }
        
        /* 表格樣式 */
        .table-container { margin-top: 20px; border-top: 2px solid #eee; padding-top: 20px; }
        table { width: 100%; border-collapse: collapse; background: #fff; font-size: 14px; }
        th, td { padding: 12px 8px; border-bottom: 1px solid #eee; text-align: center; }
        th { background-color: #f8f9fa; color: #666; }
        .btn-del { background: none; color: #e74c3c; box-shadow: none; padding: 5px; font-size: 14px; font-weight: bold; width: auto; }
    </style>
</head>
<body>
    <div class='card'>
        <div id="clock" class="time-info">系統時間: --:--:--</div>
        <h1>即時重量監控</h1>
        <div id='weight'>0.00</div>
        <div class="unit">kg</div>
        
        <div class="chart-container">
            <canvas id="realtimeChart"></canvas>
        </div>

        <div class="btn-group">
            <button class="btn-tare" onclick="tare()">歸零</button>
            <button class="btn-record" onclick="addRecord()">紀錄數據</button>
        </div>

        <div class="table-container">
            <h3 style="color:#555; font-size:16px;">持久化紀錄 (最多 10 筆)</h3>
            <table id="recordTable">
                <thead>
                    <tr>
                        <th>#</th>
                        <th>時間</th>
                        <th>重量 (kg)</th>
                        <th>操作</th>
                    </tr>
                </thead>
                <tbody id="recordBody">
                    </tbody>
            </table>
        </div>
    </div>

    <script>
        let weightChart;
        const MAX_DATA_POINTS = 30;

        // 初始化圖表
        function initChart() {
            const ctx = document.getElementById('realtimeChart').getContext('2d');
            weightChart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: Array(MAX_DATA_POINTS).fill(''),
                    datasets: [{
                        data: Array(MAX_DATA_POINTS).fill(0),
                        borderColor: '#e67e22',
                        backgroundColor: 'rgba(230, 126, 34, 0.1)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4,
                        pointRadius: 0
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: false,
                    scales: { y: { beginAtZero: false, grace: '10%' } },
                    plugins: { legend: { display: true } } // for two datasets, (false for one)
                }
            });
        }

        // --- 持久化儲存核心邏輯 ---

        // 渲染表格內容 (傳入後端回傳的 JSON 陣列)
        function renderTable(data) {
            const records = typeof data === 'string' ? JSON.parse(data) : data;
            const tbody = document.getElementById('recordBody');
            tbody.innerHTML = "";
            
            records.forEach((item, index) => {
                const row = `<tr>
                    <td>${index + 1}</td>
                    <td>${item.time}</td>
                    <td>${item.weight}</td>
                    <td><button class="btn-del" onclick="deleteRecord(${index})">✕</button></td>
                </tr>`;
                tbody.innerHTML += row;
            });
        }

        // 從 ESP32 獲取已存紀錄
        function fetchRecords() {
            fetch('/get-records').then(r => r.json()).then(renderTable);
        }

        // 新增紀錄：傳送到後端並儲存於 LittleFS
        function addRecord() {
            const timeStr = new Date().toLocaleTimeString('zh-TW', { hour12: false });
            const currentW = document.getElementById('weight').innerText;
            
            // 呼叫後端 API
            fetch(`/add-record?t=${encodeURIComponent(timeStr)}&w=${currentW}`)
                .then(r => r.json())
                .then(renderTable);
        }

        // 刪除紀錄：從後端檔案中移除
        function deleteRecord(index) {
            fetch(`/del-record?i=${index}`)
                .then(r => r.json())
                .then(renderTable);
        }

        // --- 即時更新邏輯 ---

        setInterval(function() {
            fetch('/data').then(r => r.text()).then(data => {
                const val = parseFloat(data);
                document.getElementById('weight').innerHTML = val.toFixed(2);
                weightChart.data.datasets[0].data.push(val);
                weightChart.data.datasets[0].data.shift();
                weightChart.update();
            });
        }, 500);

        setInterval(function() {
            document.getElementById('clock').innerHTML = "系統時間: " + new Date().toLocaleTimeString('zh-TW', { hour12: false });
        }, 1000);

        function tare() {
            fetch('/tare');
        }

        window.onload = function() {
            initChart();
            fetchRecords(); // 頁面載入時讀取 Flash 裡的紀錄
            fetch('/sync?t=' + Math.floor(Date.now() / 1000));
        };
    </script>
</body>
</html>
)rawliteral";
    return html;
}