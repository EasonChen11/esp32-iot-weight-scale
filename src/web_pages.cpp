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
        .btn-sched    { background-color: #9b59b6; }

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
                <button class="btn-sched" onclick="syncSheets()">Sync Sheets</button>
            </div>

            <div class="table-container">
                <h3 style="color:#555; font-size:15px;">Data Records (Max 50)</h3>
                <table>
                    <thead>
                        <tr>
                            <th>#</th>
                            <th>Date</th>
                            <th>Time</th>
                            <th>S1 (kg)</th>
                            <th>S2 (kg)</th>
                            <th>Total (kg)</th>
                            <th>Del</th>
                        </tr>
                    </thead>
                    <tbody id="recordBody"></tbody>
                </table>
                <button class="btn-clear-all" onclick="clearAll()">Clear All Records</button>
            </div>
        </div>

        <!-- ── Wake-up Schedule panel ──────────────────────────────── -->
        <div class="card">
            <h1>Wake-up Schedule</h1>
            <div class="subtitle">Daily deep-sleep wake times (max 10)</div>

            <div style="display:flex; gap:8px; justify-content:center; align-items:center; margin:16px 0;">
                <select id="schedHour" style="padding:8px; border-radius:6px; border:1px solid #ccc; font-size:15px;">
                </select>
                <span style="font-size:18px; font-weight:bold;">:</span>
                <select id="schedMin" style="padding:8px; border-radius:6px; border:1px solid #ccc; font-size:15px;">
                </select>
                <button class="btn-sched" onclick="addSchedule()">Add</button>
            </div>

            <div id="schedList" style="text-align:left; max-width:260px; margin:0 auto;"></div>
        </div>

    </div><!-- .grid -->

    <script src="/chartjs"></script>
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
                const s1 = parseFloat(item.sensor1) || 0;
                const s2 = parseFloat(item.sensor2) || 0;
                const total = (s1 + s2).toFixed(3);
                tbody.innerHTML += `<tr>
                    <td>${i + 1}</td>
                    <td>${item.date || '-'}</td>
                    <td>${item.time}</td>
                    <td>${s1.toFixed(3)}</td>
                    <td>${s2.toFixed(3)}</td>
                    <td>${total}</td>
                    <td><button class="btn-del" onclick="deleteRecord(${i})">&#x2715;</button></td>
                </tr>`;
            });
        }

        function fetchRecords() {
            fetch('/get-records').then(r => r.json()).then(renderTable);
        }

        function addRecord() {
            const now = new Date();
            const dateStr = now.toISOString().slice(0, 10);
            const timeStr = now.toLocaleTimeString('en-US', { hour12: false });
            const s1Val = document.getElementById('weight1').innerText;
            const s2Val = document.getElementById('weight2').innerText;
            fetch('/add-record?t=' + encodeURIComponent(timeStr)
                + '&d=' + dateStr + '&s1=' + s1Val + '&s2=' + s2Val)
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

        /* ── Google Sheets sync ─────────────────────────────────────── */
        function syncSheets() {
            const btn = event.target;
            btn.innerText = 'Syncing...';
            btn.disabled = true;
            fetch('/sync-sheets')
                .then(r => r.text())
                .then(msg => { alert(msg); btn.innerText = 'Sync Sheets'; btn.disabled = false; })
                .catch(() => { alert('Sync failed'); btn.innerText = 'Sync Sheets'; btn.disabled = false; });
        }

        /* ── Schedule ───────────────────────────────────────────────── */
        function populateSelects() {
            const hSel = document.getElementById('schedHour');
            const mSel = document.getElementById('schedMin');
            for (let h = 0; h < 24; h++) hSel.innerHTML += '<option value="' + h + '">' + String(h).padStart(2,'0') + '</option>';
            for (let m = 0; m < 60; m++) mSel.innerHTML += '<option value="' + m + '">' + String(m).padStart(2,'0') + '</option>';
        }

        function renderSchedule(data) {
            const items = typeof data === 'string' ? JSON.parse(data) : data;
            const el = document.getElementById('schedList');
            if (items.length === 0) { el.innerHTML = '<p style="color:#95a5a6; text-align:center; font-size:13px;">No wake-up times set</p>'; return; }
            let html = '';
            items.forEach((s, i) => {
                const t = String(s.h).padStart(2,'0') + ':' + String(s.m).padStart(2,'0');
                html += '<div style="display:flex; justify-content:space-between; align-items:center; padding:8px 0; border-bottom:1px solid #eee;">'
                    + '<span style="font-size:18px; font-weight:bold; color:#9b59b6;">' + t + '</span>'
                    + '<button class="btn-del" onclick="delSchedule(' + i + ')">&#x2715;</button></div>';
            });
            el.innerHTML = html;
        }

        function fetchSchedule() {
            fetch('/get-schedule').then(r => r.json()).then(renderSchedule);
        }

        function addSchedule() {
            const h = document.getElementById('schedHour').value;
            const m = document.getElementById('schedMin').value;
            fetch('/add-schedule?h=' + h + '&m=' + m)
                .then(r => { if (!r.ok) throw new Error('duplicate'); return r.json(); })
                .then(renderSchedule)
                .catch(() => alert('Invalid or duplicate time'));
        }

        function delSchedule(i) {
            fetch('/del-schedule?i=' + i).then(r => r.json()).then(renderSchedule);
        }

        /* ── Init ───────────────────────────────────────────────────── */
        window.onload = function() {
            populateSelects();
            fetchSchedule();
            fetchRecords();

            if (typeof Chart !== 'undefined') {
                chartTotal = makeChart('chartTotal', '#27ae60');
            } else {
                console.error('Chart.js not loaded — upload LittleFS image');
            }

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

#if WIFI_CONFIG_ENABLED
/*
WiFi network configuration subpage. Reachable from the main dashboard's
"⚙ Network" link. Lets the user scan, select, enter a password, and connect
to a new SSID at runtime. Status is fetched from /wifi-status (JSON) and
the result of a connect attempt is observed by polling /wifi-status at 1 Hz.
*/
String getNetworkPageHTML()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <meta charset='UTF-8'>
  <title>WiFi Network Settings</title>
  <style>
    * { box-sizing: border-box; }
    body { font-family: Arial, sans-serif; background: #f4f4f4;
           padding: 16px; margin: 0; max-width: 600px; margin: 0 auto; }
    a.back { display: inline-block; margin-bottom: 16px;
             color: #3498db; text-decoration: none; font-size: 14px; }
    h1 { color: #333; font-size: 22px; margin: 0 0 16px; }
    .card { background: white; padding: 18px; border-radius: 14px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1); margin-bottom: 16px; }
    .card h2 { margin: 0 0 12px; font-size: 15px; color: #555;
               display: flex; justify-content: space-between; align-items: center; }
    .row { display: flex; justify-content: space-between;
           padding: 6px 0; font-size: 14px; }
    .row .k { color: #7f8c8d; }
    .dot { width: 10px; height: 10px; border-radius: 50%;
           display: inline-block; margin-right: 6px; vertical-align: middle; }
    .dot-connected    { background: #27ae60; }
    .dot-connecting   { background: #f39c12; }
    .dot-disconnected { background: #95a5a6; }
    .dot-failed       { background: #e74c3c; }

    #scanList { max-height: 280px; overflow-y: auto; }
    .ap { display: flex; justify-content: space-between; align-items: center;
          padding: 10px 8px; border-bottom: 1px solid #eee;
          cursor: pointer; font-size: 14px; }
    .ap:hover { background: #f8f9fa; }
    .ap.selected { background: #e8f4fc; }
    .ap .ssid { font-weight: bold; color: #333; }
    .ap .meta { color: #95a5a6; font-size: 12px; }
    .bars { display: inline-block; width: 32px; text-align: right;
            color: #3498db; font-family: monospace; }

    .field { margin: 10px 0; }
    .field label { display: block; font-size: 13px; color: #7f8c8d;
                   margin-bottom: 4px; }
    .field input { width: 100%; padding: 10px; font-size: 15px;
                   border: 1px solid #ccc; border-radius: 6px; outline: none; }
    .pwrow { display: flex; gap: 6px; }
    .pwrow input { flex: 1; }
    .pwrow button.eye { background: #ecf0f1; color: #555; border: none;
                        padding: 0 12px; border-radius: 6px; cursor: pointer; }

    button.primary { width: 100%; background: #27ae60; color: white;
                     border: none; padding: 12px; font-size: 15px;
                     border-radius: 8px; cursor: pointer;
                     box-shadow: 0 4px rgba(0,0,0,0.1); }
    button.primary:disabled { background: #bdc3c7; cursor: not-allowed; }
    button.danger { width: 100%; background: white; color: #e74c3c;
                    border: 1px solid #e74c3c; padding: 10px;
                    font-size: 13px; border-radius: 6px; cursor: pointer; }
    button.scan { background: none; border: 1px solid #3498db; color: #3498db;
                  padding: 4px 10px; border-radius: 4px; cursor: pointer;
                  font-size: 12px; }

    #msg { padding: 10px; border-radius: 6px; font-size: 14px;
           margin-top: 10px; display: none; }
    #msg.ok    { display: block; background: #d4edda; color: #155724; }
    #msg.err   { display: block; background: #f8d7da; color: #721c24; }
    #msg.info  { display: block; background: #d1ecf1; color: #0c5460; }
    .danger-zone { border-top: 2px dashed #e74c3c; padding-top: 14px;
                   margin-top: 14px; }
    .danger-zone p { color: #95a5a6; font-size: 12px; margin: 6px 0; }
  </style>
</head>
<body>
  <a class="back" href="/">&larr; Back to Dashboard</a>
  <h1>WiFi Network Settings</h1>

  <div class="card">
    <h2>Current Status</h2>
    <div class="row"><span class="k">Status</span>
      <span><span id="curDot" class="dot dot-disconnected"></span><span id="curStatus">--</span></span></div>
    <div class="row"><span class="k">SSID</span><span id="curSsid">--</span></div>
    <div class="row"><span class="k">IP</span><span id="curIp">--</span></div>
    <div class="row"><span class="k">Signal</span><span id="curRssi">--</span></div>
  </div>

  <div class="card">
    <h2>Available Networks <button class="scan" onclick="doScan()">&#x21bb; Rescan</button></h2>
    <div id="scanList"><p style="color:#95a5a6; text-align:center;">Scanning...</p></div>
  </div>

  <div class="card" id="formCard" style="display:none;">
    <h2>Connect</h2>
    <div class="field">
      <label>SSID</label>
      <input id="fSsid" type="text" placeholder="Network name">
    </div>
    <div class="field" id="passField">
      <label>Password</label>
      <div class="pwrow">
        <input id="fPass" type="password" placeholder="8-63 characters">
        <button class="eye" type="button" onclick="togglePw()">&#128065;</button>
      </div>
    </div>
    <button id="connectBtn" class="primary" onclick="doConnect()">Connect</button>
    <div id="msg"></div>
  </div>

  <div class="card danger-zone">
    <h2>Danger Zone</h2>
    <button class="danger" onclick="doClear()">Clear Stored Network</button>
    <p>Wipes saved credentials. ESP32 reverts to default network on next reboot.</p>
  </div>

<script>
let pollTimer = null;

function $(id) { return document.getElementById(id); }

function show(elemId, cls, text) {
  const el = $(elemId);
  el.className = cls;
  el.innerText = text;
}

function refreshStatus() {
  return fetch('/wifi-status').then(r => r.json()).then(s => {
    $('curDot').className = 'dot dot-' + s.status;
    $('curStatus').innerText = s.status;
    $('curSsid').innerText  = s.current_ssid || (s.target_ssid ? '(connecting to ' + s.target_ssid + ')' : '--');
    $('curIp').innerText    = s.ip   || '--';
    $('curRssi').innerText  = (s.rssi !== undefined) ? (s.rssi + ' dBm') : '--';
    return s;
  });
}

function rssiBars(rssi) {
  if (rssi >= -50) return '&#9646;&#9646;&#9646;&#9646;';
  if (rssi >= -65) return '&#9646;&#9646;&#9646;&#9647;';
  if (rssi >= -75) return '&#9646;&#9646;&#9647;&#9647;';
  return '&#9646;&#9647;&#9647;&#9647;';
}

function renderScan(list) {
  const el = $('scanList');
  if (list.length === 0) {
    el.innerHTML = '<p style="color:#e74c3c; text-align:center;">&#9888; No networks found nearby</p>';
    $('formCard').style.display = 'block';
    $('fSsid').value = '';
    $('fSsid').focus();
    return;
  }
  let html = '';
  list.forEach((ap, i) => {
    const lock = (ap.enc === 'OPEN') ? '' : '&#128274;';
    html += '<div class="ap" data-i="' + i + '" onclick="selectAp(' + i + ')">'
         +    '<div><span class="ssid">' + ap.ssid + '</span> '
         +      '<span class="meta">' + lock + '</span></div>'
         +    '<div><span class="bars">' + rssiBars(ap.rssi) + '</span> '
         +      '<span class="meta">' + ap.rssi + ' dBm</span></div>'
         +  '</div>';
  });
  html += '<div class="ap" onclick="selectOther()" style="color:#3498db;">'
       +    '<div>&#9656; Other (hidden network)</div></div>';
  el.innerHTML = html;
  window.__scanList = list;
}

function selectAp(i) {
  document.querySelectorAll('.ap').forEach(e => e.classList.remove('selected'));
  const target = document.querySelector('.ap[data-i="' + i + '"]');
  if (target) target.classList.add('selected');
  const ap = window.__scanList[i];
  $('formCard').style.display = 'block';
  $('fSsid').value = ap.ssid;
  $('fSsid').readOnly = true;
  if (ap.enc === 'OPEN') {
    $('passField').style.display = 'none';
    $('fPass').value = '';
  } else {
    $('passField').style.display = 'block';
    $('fPass').value = '';
    $('fPass').focus();
  }
}

function selectOther() {
  $('formCard').style.display = 'block';
  $('fSsid').readOnly = false;
  $('fSsid').value = '';
  $('passField').style.display = 'block';
  $('fPass').value = '';
  $('fSsid').focus();
}

function togglePw() {
  const f = $('fPass');
  f.type = (f.type === 'password') ? 'text' : 'password';
}

function doScan() {
  $('scanList').innerHTML = '<p style="color:#95a5a6; text-align:center;">Scanning...</p>';
  fetch('/network/scan').then(r => {
    if (r.status === 409) { show('msg', 'err', 'WiFi busy, try again shortly'); return null; }
    if (!r.ok) { show('msg', 'err', 'Scan failed (' + r.status + ')'); return null; }
    return r.json();
  }).then(list => { if (list) renderScan(list); });
}

function startPolling() {
  if (pollTimer) return;
  let elapsed = 0;
  pollTimer = setInterval(() => {
    elapsed += 1;
    refreshStatus().then(s => {
      if (s.status === 'connected') {
        clearInterval(pollTimer); pollTimer = null;
        show('msg', 'ok', '\u2713 Connected to ' + s.current_ssid + ' (' + s.ip + ')');
        $('connectBtn').disabled = false;
        $('connectBtn').innerText = 'Connect';
      } else if (s.status === 'failed' || elapsed >= 10) {
        clearInterval(pollTimer); pollTimer = null;
        show('msg', 'err', '\u2717 Failed to connect — wrong password or out of range');
        $('fPass').value = '';
        $('connectBtn').disabled = false;
        $('connectBtn').innerText = 'Connect';
      }
    });
  }, 1000);
}

function doConnect() {
  const ssid = $('fSsid').value.trim();
  const pass = $('fPass').value;
  if (!ssid) { show('msg', 'err', 'SSID required'); return; }
  if ($('passField').style.display !== 'none' && pass.length > 0 &&
      (pass.length < 8 || pass.length > 63)) {
    show('msg', 'err', 'Password must be 8-63 characters'); return;
  }

  $('connectBtn').disabled = true;
  $('connectBtn').innerText = 'Connecting...';
  show('msg', 'info', 'Connecting to ' + ssid + '...');

  const body = 's=' + encodeURIComponent(ssid) + '&p=' + encodeURIComponent(pass);
  fetch('/network/save', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  }).then(r => {
    if (r.status === 202) { startPolling(); }
    else if (r.status === 409) {
      r.json().then(d => {
        show('msg', 'info', 'Already connecting to ' + (d.current_ssid || 'network'));
        startPolling();
      });
    } else if (r.status === 400) {
      r.json().then(d => {
        show('msg', 'err', d.error || 'Bad request');
        $('connectBtn').disabled = false;
        $('connectBtn').innerText = 'Connect';
      });
    } else {
      show('msg', 'err', 'Server error (' + r.status + ')');
      $('connectBtn').disabled = false;
      $('connectBtn').innerText = 'Connect';
    }
  });
}

function doClear() {
  if (!confirm('Clear stored WiFi credentials? On next reboot, ESP32 will use the default network.')) return;
  fetch('/network/clear', { method: 'POST' }).then(r => r.json()).then(d => {
    show('msg', 'ok', 'Stored credentials cleared. Current connection unchanged.');
  });
}

window.onload = function() {
  refreshStatus().then(s => {
    if (s.status === 'connecting') {
      $('formCard').style.display = 'block';
      $('connectBtn').disabled = true;
      $('connectBtn').innerText = 'Connecting to ' + (s.target_ssid || '...');
      startPolling();
    }
    doScan();
  });
};
</script>
</body>
</html>
)rawliteral";
    return html;
}
#endif // WIFI_CONFIG_ENABLED
