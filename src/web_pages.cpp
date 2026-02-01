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
        
        /* 刪除全部按鈕樣式 */
        .btn-clear-all { 
            background-color: transparent; 
            color: #95a5a6; 
            border: 1px solid #bdc3c7; 
            margin-top: 15px; 
            padding: 8px 16px; 
            font-size: 13px; 
            width: 100%; 
            box-shadow: none;
        }
        .btn-clear-all:hover { background-color: #f8f9fa; color: #e74c3c; border-color: #e74c3c; }

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
        <h1>重量監測</h1>
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
            <h3 style="color:#555; font-size:16px;">數據紀錄 (最多 10 筆)</h3>
            <table id="recordTable">
                <thead>
                    <tr>
                        <th>#</th>
                        <th>時間</th>
                        <th>重量 (kg)</th>
                        <th>刪除</th>
                    </tr>
                </thead>
                <tbody id="recordBody">
                </tbody>
            </table>
            <button class="btn-clear-all" onclick="clearAll()">清空所有紀錄</button>

            <div style="margin-top: 40px; padding: 15px; border: 1px dashed #e74c3c; border-radius: 12px; background-color: #fffafa;">
                <p style="color: #7f8c8d; font-size: 11px; margin-bottom: 10px;">僅在秤盤完全清空時，用於絕對零點校正。</p>
                <button style="background: white; color: #e74c3c; border: 1px solid #e74c3c; padding: 8px 16px; font-size: 13px; border-radius: 6px; width: 100%; box-shadow: none;" 
                        onclick="setAbsoluteZero()">執行絕對零點校正</button>
            </div>
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
                    plugins: { legend: { display: false } }
                }
            });
        }

        // --- 持久化儲存核心邏輯 ---

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

        function fetchRecords() {
            fetch('/get-records').then(r => r.json()).then(renderTable);
        }

        function addRecord() {
            const timeStr = new Date().toLocaleTimeString('zh-TW', { hour12: false });
            const currentW = document.getElementById('weight').innerText;
            fetch(`/add-record?t=${encodeURIComponent(timeStr)}&w=${currentW}`)
                .then(r => r.json())
                .then(renderTable);
        }

        function deleteRecord(index) {
            fetch(`/del-record?i=${index}`)
                .then(r => r.json())
                .then(renderTable);
        }

        // --- 新增：清空所有紀錄函式 ---
        function clearAll() {
            if (confirm("確定要刪除所有歷史紀錄嗎？這動作無法復原。")) {
                fetch('/clear-records')
                    .then(r => r.json())
                    .then(renderTable)
                    .catch(err => console.error("清除失敗", err));
            }
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
            fetchRecords(); 
            fetch('/sync?t=' + Math.floor(Date.now() / 1000));
        };
        function setAbsoluteZero() {
        // 增加一個確認步驟，防止誤觸
        const code = prompt("【工程師校正】請確認秤盤已完全清空，並輸入 'RESET' 執行絕對零點校正：");
        
        if (code === "RESET") {
            const btn = event.target;
            const originalText = btn.innerHTML;
            btn.innerHTML = "校正中...";
            btn.disabled = true;

            // 使用 fetch 在背景發送請求，不跳轉頁面
            fetch('/set-zero')
                .then(response => response.text())
                .then(data => {
                    alert("絕對零點校正成功！新 Offset 已存入 Flash。");
                    btn.innerHTML = originalText;
                    btn.disabled = false;
                })
                .catch(err => {
                    alert("校正失敗，請檢查連線");
                    btn.innerHTML = originalText;
                    btn.disabled = false;
                });
        } else if (code !== null) {
            alert("指令錯誤，校正取消。");
        }
    }
    </script>
</body>
</html>
)rawliteral";
    return html;
}