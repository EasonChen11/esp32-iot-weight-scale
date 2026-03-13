#include "web_pages.h"

/*
Generate the complete HTML content for the main dashboard page.
Three panels: Sensor 1, Sensor 2, and Total (beehive) weight.
Each panel has a live chart; the Total panel also has the records table.
*/
String getIndexHTML()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <meta charset='UTF-8'>
    <title>ESP32 Beehive Weight Monitor</title>
    <script src="/chartjs"></script>
    <style>
        * { box-sizing: border-box; }
        body { font-family: Arial, sans-serif; text-align: center;
               background: #f4f4f4; padding: 16px; margin: 0; }

        #clock { color: #95a5a6; font-size: 13px; margin-bottom: 16px; }

        /* Responsive grid: 1 col on mobile, 3 cols on wider screens */
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 16px;
            max-width: 1100px;
            margin: 0 auto;
        }

        .card {
            background: white;
            padding: 20px;
            border-radius: 20px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
        }

        h1 { color: #333; font-size: 20px; margin: 0 0 4px; }
        .subtitle { color: #95a5a6; font-size: 12px; margin-bottom: 10px; }

        .weight-display { font-size: 54px; font-weight: bold; margin: 8px 0 0; }
        .unit { color: #7f8c8d; font-size: 17px; margin-bottom: 10px; }

        .s1-color  { color: #e67e22; }
        .s2-color  { color: #3498db; }
        .tot-color { color: #27ae60; }

        .chart-container { height: 160px; width: 100%; margin-bottom: 16px; }

        .btn-group { display: flex; gap: 10px; justify-content: center; margin-bottom: 16px; }
        button {
            border: none; color: white; padding: 11px 22px;
            font-size: 15px; border-radius: 8px; cursor: pointer;
            transition: all 0.2s; box-shadow: 0 4px rgba(0,0,0,0.1); outline: none;
        }
        button:active { transform: translateY(2px); box-shadow: 0 2px rgba(0,0,0,0.1); }

        .btn-tare-s1  { background-color: #e67e22; }
        .btn-tare-s2  { background-color: #3498db; }
        .btn-record   { background-color: #27ae60; }

        .btn-clear-all {
            background-color: transparent; color: #95a5a6;
            border: 1px solid #bdc3c7; margin-top: 12px;
            padding: 8px 16px; font-size: 13px; width: 100%; box-shadow: none;
        }
        .btn-clear-all:hover { background-color: #f8f9fa; color: #e74c3c; border-color: #e74c3c; }

        .table-container { margin-top: 16px; border-top: 2px solid #eee; padding-top: 16px; }
        table { width: 100%; border-collapse: collapse; font-size: 14px; }
        th, td { padding: 10px 6px; border-bottom: 1px solid #eee; text-align: center; }
        th { background-color: #f8f9fa; color: #666; }
        .btn-del { background: none; color: #e74c3c; box-shadow: none;
                   padding: 4px; font-size: 14px; width: auto; }

        .calibration-panel { margin-top: 14px; text-align: left; }
        .calibration-panel summary {
            cursor: pointer; list-style: none; font-size: 12px;
            color: #e74c3c; font-weight: bold;
        }
        .calibration-panel summary::-webkit-details-marker { display: none; }
        .calibration-content {
            margin-top: 8px; padding: 10px;
            border: 1px dashed #e74c3c; border-radius: 10px; background: #fffafa;
        }
        .calibration-note { color: #7f8c8d; font-size: 11px; margin-bottom: 8px; }
        .btn-cal {
            background: white; color: #e74c3c; border: 1px solid #e74c3c;
            padding: 7px 14px; font-size: 12px; border-radius: 6px;
            width: 100%; box-shadow: none;
        }
    </style>
</head>
<body>
    <div id="clock">System time: --:--:--</div>

    <div class="grid">

        <!-- ── Sensor 1 panel ──────────────────────────────────────── -->
        <div class="card">
            <h1>Sensor 1</h1>
            <div class="subtitle">Left side load cell</div>
            <div id="weight1" class="weight-display s1-color">0.00</div>
            <div class="unit">kg</div>
            <div class="btn-group">
                <button class="btn-tare-s1" onclick="tare1()">Tare S1</button>
            </div>
            <details class="calibration-panel">
                <summary>Absolute Zero Calibration (S1)</summary>
                <div class="calibration-content">
                    <p class="calibration-note">Perform only when Sensor 1 is completely unloaded.</p>
                    <button class="btn-cal" onclick="setZero(1, event)">Calibrate Sensor 1 Zero</button>
                </div>
            </details>
        </div>

        <!-- ── Sensor 2 panel ──────────────────────────────────────── -->
        <div class="card">
            <h1>Sensor 2</h1>
            <div class="subtitle">Right side load cell</div>
            <div id="weight2" class="weight-display s2-color">0.00</div>
            <div class="unit">kg</div>
            <div class="btn-group">
                <button class="btn-tare-s2" onclick="tare2()">Tare S2</button>
            </div>
            <details class="calibration-panel">
                <summary>Absolute Zero Calibration (S2)</summary>
                <div class="calibration-content">
                    <p class="calibration-note">Perform only when Sensor 2 is completely unloaded.</p>
                    <button class="btn-cal" onclick="setZero(2, event)">Calibrate Sensor 2 Zero</button>
                </div>
            </details>
        </div>

        <!-- ── Total panel ─────────────────────────────────────────── -->
        <div class="card">
            <h1>Total Weight</h1>
            <div class="subtitle">Beehive (S1 + S2)</div>
            <div id="weightTotal" class="weight-display tot-color">0.00</div>
            <div class="unit">kg</div>
            <div class="chart-container">
                <canvas id="chartTotal"></canvas>
            </div>
            <div class="btn-group">
                <button class="btn-record" onclick="addRecord()">Record Data</button>
            </div>

            <div class="table-container">
                <h3 style="color:#555; font-size:15px;">Data Records (Max 10)</h3>
                <table>
                    <thead>
                        <tr>
                            <th>#</th>
                            <th>Time</th>
                            <th>Weight (kg)</th>
                            <th>Del</th>
                        </tr>
                    </thead>
                    <tbody id="recordBody"></tbody>
                </table>
                <button class="btn-clear-all" onclick="clearAll()">Clear All Records</button>
            </div>
        </div>

    </div><!-- .grid -->

    <script>
        const MAX_PTS = 30;
        let chartTotal;

        /* ── Chart factory ──────────────────────────────────────────── */
        function makeChart(id, color) {
            const ctx = document.getElementById(id).getContext('2d');
            return new Chart(ctx, {
                type: 'line',
                data: {
                    labels: Array(MAX_PTS).fill(''),
                    datasets: [{
                        data: Array(MAX_PTS).fill(0),
                        borderColor: color,
                        backgroundColor: color + '1a',
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

        function pushChart(chart, val) {
            chart.data.datasets[0].data.push(val);
            chart.data.datasets[0].data.shift();
            chart.update();
        }

        /* ── Data polling (500 ms) ──────────────────────────────────── */
        setInterval(function() {
            Promise.all([
                fetch('/data1').then(r => r.text()),
                fetch('/data2').then(r => r.text())
            ]).then(([d1, d2]) => {
                const v1    = parseFloat(d1) || 0;
                const v2    = parseFloat(d2) || 0;
                const total = parseFloat((v1 + v2).toFixed(2));

                document.getElementById('weight1').innerText     = v1.toFixed(2);
                document.getElementById('weight2').innerText     = v2.toFixed(2);
                document.getElementById('weightTotal').innerText = total.toFixed(2);

                if (chartTotal) pushChart(chartTotal, total);
            }).catch(() => {});
        }, 500);

        /* ── Clock ──────────────────────────────────────────────────── */
        setInterval(function() {
            document.getElementById('clock').innerText =
                'System time: ' + new Date().toLocaleTimeString('en-US', { hour12: false });
        }, 1000);

        /* ── Tare ───────────────────────────────────────────────────── */
        function tare1() { fetch('/tare1'); }
        function tare2() { fetch('/tare2'); }

        /* ── Absolute zero calibration ──────────────────────────────── */
        function setZero(sensor, event) {
            const code = prompt("Confirm sensor " + sensor + " is completely unloaded, then enter 'RESET':");
            if (code !== "RESET") {
                if (code !== null) alert("Incorrect code — calibration cancelled.");
                return;
            }
            const btn = event.target;
            const orig = btn.innerHTML;
            btn.innerHTML = "Calibrating...";
            btn.disabled = true;
            fetch('/set-zero' + sensor)
                .then(r => r.text())
                .then(() => {
                    alert("Sensor " + sensor + " calibration saved.");
                    btn.innerHTML = orig;
                    btn.disabled = false;
                })
                .catch(() => {
                    alert("Calibration failed — check connection.");
                    btn.innerHTML = orig;
                    btn.disabled = false;
                });
        }

        /* ── Records ────────────────────────────────────────────────── */
        function renderTable(data) {
            const records = typeof data === 'string' ? JSON.parse(data) : data;
            const tbody = document.getElementById('recordBody');
            tbody.innerHTML = '';
            records.forEach((item, i) => {
                tbody.innerHTML += `<tr>
                    <td>${i + 1}</td>
                    <td>${item.time}</td>
                    <td>${item.weight}</td>
                    <td><button class="btn-del" onclick="deleteRecord(${i})">&#x2715;</button></td>
                </tr>`;
            });
        }

        function fetchRecords() {
            fetch('/get-records').then(r => r.json()).then(renderTable);
        }

        function addRecord() {
            const timeStr  = new Date().toLocaleTimeString('en-US', { hour12: false });
            const totalVal = document.getElementById('weightTotal').innerText;
            fetch('/add-record?t=' + encodeURIComponent(timeStr) + '&w=' + totalVal)
                .then(r => r.json()).then(renderTable);
        }

        function deleteRecord(index) {
            fetch('/del-record?i=' + index).then(r => r.json()).then(renderTable);
        }

        function clearAll() {
            if (confirm("Delete all historical records? This cannot be undone.")) {
                fetch('/clear-records').then(r => r.json()).then(renderTable)
                    .catch(err => console.error("Clear failed", err));
            }
        }

        /* ── Init ───────────────────────────────────────────────────── */
        window.onload = function() {
            chartTotal  = makeChart('chartTotal',  '#27ae60');

            fetchRecords();

            const timestamp = Math.floor(Date.now() / 1000);
            fetch('/sync?t=' + timestamp).then(response => {
                if (response.ok) {
                    setTimeout(fetchRecords, 1500);
                }
            });

            setInterval(fetchRecords, 60000);
        };
    </script>
</body>
</html>
)rawliteral";
    return html;
}
