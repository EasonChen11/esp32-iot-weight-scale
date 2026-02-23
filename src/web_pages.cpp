#include "web_pages.h"

/*
Generate the complete HTML content for the main dashboard page.
This function returns the full HTML/CSS/JavaScript code for the weight monitoring
dashboard, including real-time charts, data tables, and control buttons.
The dashboard provides live weight display, historical data management, and sensor calibration controls.

Parameters:
  none

Returns:
  String: Complete HTML document as a single string
*/
String getIndexHTML()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <meta charset='UTF-8'>
    <title>ESP32 Weight Scale Monitor</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f4; padding: 20px; margin: 0; }
        .card { background: white; padding: 20px; border-radius: 20px; display: inline-block; box-shadow: 0 4px 15px rgba(0,0,0,0.1); width: 95%; max-width: 500px; }
        h1 { color: #333; font-size: 22px; margin-bottom: 5px; }
        #weight { font-size: 56px; color: #e67e22; font-weight: bold; margin: 10px 0; }
        .unit { color: #7f8c8d; font-size: 18px; margin-bottom: 10px; }
        .time-info { color: #95a5a6; font-size: 13px; margin-bottom: 15px; }
        
        .chart-container { height: 180px; width: 100%; margin-bottom: 20px; }

        .btn-group { display: flex; gap: 10px; justify-content: center; margin-bottom: 25px; }
        button { border: none; color: white; padding: 12px 24px; font-size: 16px; border-radius: 8px; cursor: pointer; transition: all 0.2s; box-shadow: 0 4px rgba(0,0,0,0.1); outline: none; }
        button:active { transform: translateY(2px); box-shadow: 0 2px rgba(0,0,0,0.1); }
        
        .btn-tare { background-color: #e74c3c; } 
        .btn-record { background-color: #2ecc71; }
        
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

        .table-container { margin-top: 20px; border-top: 2px solid #eee; padding-top: 20px; }
        table { width: 100%; border-collapse: collapse; background: #fff; font-size: 14px; }
        th, td { padding: 12px 8px; border-bottom: 1px solid #eee; text-align: center; }
        th { background-color: #f8f9fa; color: #666; }
        .btn-del { background: none; color: #e74c3c; box-shadow: none; padding: 5px; font-size: 14px; font-weight: bold; width: auto; }
        .calibration-panel { margin-top: 18px; text-align: left; }
        .calibration-panel summary { cursor: pointer; list-style: none; font-size: 13px; color: #e74c3c; font-weight: bold; }
        .calibration-panel summary::-webkit-details-marker { display: none; }
        .calibration-content { margin-top: 10px; padding: 12px; border: 1px dashed #e74c3c; border-radius: 12px; background-color: #fffafa; }
        .calibration-note { color: #7f8c8d; font-size: 11px; margin-bottom: 10px; }
    </style>
</head>
<body>
    <div class='card'>
        <div id="clock" class="time-info">System time: --:--:--</div>
        <h1>Weight Measurement</h1>
        <div id='weight'>0.00</div>
        <div class="unit">kg</div>
        
        <div class="chart-container">
            <canvas id="realtimeChart"></canvas>
        </div>

        <div class="btn-group">
            <button class="btn-tare" onclick="tare()">Tare</button>
            <button class="btn-record" onclick="addRecord()">Record Data</button>
        </div>

        <div class="table-container">
            <h3 style="color:#555; font-size:16px;">Data Records (Max 10)</h3>
            <table id="recordTable">
                <thead>
                    <tr>
                        <th>#</th>
                        <th>Time</th>
                        <th>Weight (kg)</th>
                        <th>Delete</th>
                    </tr>
                </thead>
                <tbody id="recordBody">
                </tbody>
            </table>
            <button class="btn-clear-all" onclick="clearAll()">Clear All Records</button>

            <details class="calibration-panel">
                <summary>Absolute Zero Calibration</summary>
                <div class="calibration-content">
                    <p class="calibration-note">Perform only when the scale is completely empty.</p>
                    <button style="background: white; color: #e74c3c; border: 1px solid #e74c3c; padding: 8px 16px; font-size: 13px; border-radius: 6px; width: 100%; box-shadow: none;" 
                            onclick="setAbsoluteZero()">Calibrate Absolute Zero</button>
                </div>
            </details>
        </div>
    </div>

    <script>
        let weightChart;
        const MAX_DATA_POINTS = 30;

        /*
        Initialize the real-time weight chart.
        Sets up a line chart with 30 data points for continuous weight monitoring.
        */
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

        /*
        Render the data records table from JSON data.
        Populates the table with all stored weight measurements.
        */
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

        /*
        Fetch all stored records from the server.
        */
        function fetchRecords() {
            fetch('/get-records').then(r => r.json()).then(renderTable);
        }

        /*
        Add a new weight measurement record.
        Captures current timestamp and weight value, sends to server for persistent storage.
        */
        function addRecord() {
            const timeStr = new Date().toLocaleTimeString('en-US', { hour12: false });
            const currentW = document.getElementById('weight').innerText;
            fetch(`/add-record?t=${encodeURIComponent(timeStr)}&w=${currentW}`)
                .then(r => r.json())
                .then(renderTable);
        }

        /*
        Delete a specific record by index.
        */
        function deleteRecord(index) {
            fetch(`/del-record?i=${index}`)
                .then(r => r.json())
                .then(renderTable);
        }

        /*
        Clear all stored records after user confirmation.
        */
        function clearAll() {
            if (confirm("Delete all historical records? This action cannot be undone.")) {
                fetch('/clear-records')
                    .then(r => r.json())
                    .then(renderTable)
                    .catch(err => console.error("Clear failed", err));
            }
        }

        /*
        Update weight display and chart every 500ms.
        Fetches current weight reading from server and updates visualization.
        */
        setInterval(function() {
            fetch('/data').then(r => r.text()).then(data => {
                const val = parseFloat(data);
                document.getElementById('weight').innerHTML = val.toFixed(2);
                weightChart.data.datasets[0].data.push(val);
                weightChart.data.datasets[0].data.shift();
                weightChart.update();
            });
        }, 500);

        /*
        Update system clock display every 1 second.
        */
        setInterval(function() {
            document.getElementById('clock').innerHTML = "System time: " + new Date().toLocaleTimeString('en-US', { hour12: false });
        }, 1000);

        /*
        Perform temporary tare (zero adjustment) on the scale.
        */
        function tare() {
            fetch('/tare');
        }

        /*
        Perform absolute zero-point calibration.
        Requires user confirmation and verification code to prevent accidental calibration.
        */
        function setAbsoluteZero() {
            const code = prompt("Confirm scale is completely empty and enter 'RESET' to calibrate:");
            
            if (code === "RESET") {
                const btn = event.target;
                const originalText = btn.innerHTML;
                btn.innerHTML = "Calibrating...";
                btn.disabled = true;

                fetch('/set-zero')
                    .then(response => response.text())
                    .then(data => {
                        alert("Calibration successful! New offset saved to storage.");
                        btn.innerHTML = originalText;
                        btn.disabled = false;
                    })
                    .catch(err => {
                        alert("Calibration failed, check connection");
                        btn.innerHTML = originalText;
                        btn.disabled = false;
                    });
            } else if (code !== null) {
                alert("Incorrect code, calibration cancelled.");
            }
        }

        /*
        Initialize dashboard on page load.
        Sets up chart, loads stored records, and synchronizes system time.
        */
        window.onload = function() {
            initChart();
            
            // 1. 先抓取一次現有的紀錄 (舊資料)
            fetchRecords(); 

            // 2. 獲取當前時間戳並同步
            const timestamp = Math.floor(Date.now() / 1000);
            fetch('/sync?t=' + timestamp)
                .then(response => {
                    if (response.ok) {
                        // 3. 關鍵：等待 1.5 秒，讓 ESP32 Core 0 有足夠時間完成檔案寫入
                        // 這樣「初始紀錄」才會出現在接下來抓取的資料中
                        setTimeout(fetchRecords, 1500); 
                    }
                });

            // 4. 每 60 秒自動刷新一次表格 (為了顯示每小時的自動紀錄)
            setInterval(fetchRecords, 60000);
        };
    </script>
</body>
</html>
)rawliteral";
    return html;
}