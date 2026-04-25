#include "web_pages.h"
#include "config.h"
#if DEV_MODE_ENABLED
#include "dev_mode.h"
#endif

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
    <title>ESP32 蜂箱重量監控</title>
    <style>
        * { box-sizing: border-box; }
        body { font-family: Arial, sans-serif; text-align: center;
               background: #f4f4f4; padding: 16px; margin: 0; }

        #clock { color: #95a5a6; font-size: 13px; margin-bottom: 16px; }

        #netbar { display: flex; align-items: center; gap: 8px;
                  font-size: 13px; color: #7f8c8d;
                  max-width: 1100px; margin: 0 auto 8px;
                  padding: 0 4px; }
        #netbar .dot { width: 10px; height: 10px; border-radius: 50%;
                       display: inline-block; }
        .dot-connected    { background: #27ae60;
                            animation: heartbeat 1.5s ease-in-out infinite; }
        .dot-connecting   { background: #f39c12;
                            animation: heartbeat 0.5s ease-in-out infinite; }
        .dot-disconnected { background: #95a5a6; }
        .dot-failed       { background: #e74c3c; }
        @keyframes heartbeat {
            0%, 100% { opacity: 1;   transform: scale(1); }
            50%      { opacity: 0.4; transform: scale(0.85); }
        }
        #netLink { margin-left: auto; color: #3498db; text-decoration: none;
                   padding: 2px 10px; border: 1px solid #3498db; border-radius: 4px; }
        #netLink:hover { background: #3498db; color: white; }

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
        .table-scroll { max-height: 220px; overflow-y: auto; border: 1px solid #eee; border-radius: 8px; }
        table { width: 100%; border-collapse: collapse; font-size: 14px; }
        th, td { padding: 10px 6px; border-bottom: 1px solid #eee; text-align: center; }
        th { background-color: #f8f9fa; color: #666; position: sticky; top: 0; z-index: 1; }
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

        .cal-section-title {
            margin: 12px 0 4px;
            font-size: 12px;
            color: #e74c3c;
            font-weight: bold;
            border-top: 1px dashed #f0c0c0;
            padding-top: 8px;
        }
        .cal-section-title:first-of-type {
            border-top: none;
            padding-top: 0;
            margin-top: 0;
        }

        .cal-row { display: flex; gap: 6px; margin-top: 6px; }
        .cal-weight-input {
            flex: 1; padding: 7px; font-size: 12px;
            border: 1px solid #ccc; border-radius: 6px; outline: none;
        }
        .cal-row .btn-cal { width: auto; padding: 7px 12px; flex-shrink: 0; }

        .cal-status {
            margin: 6px 0 0; font-size: 11px; color: #7f8c8d;
            text-align: center; min-height: 14px;
        }
        .cal-status.success { color: #27ae60; }
        .cal-status.error   { color: #e74c3c; }
    </style>
</head>
<body>
{{DEV_BANNER}}
    <div id="netbar">
        <span id="netDot" class="dot dot-disconnected"></span>
        <span id="netSsid">--</span>
        <span id="netIp"></span>
        <a href="/network" id="netLink">&#9881; 網路</a>
    </div>
    <div id="clock">系統時間：--:--:--</div>

    <div class="grid">

        <!-- ── Sensor 1 panel ──────────────────────────────────────── -->
        <div class="card">
            <h1>感測器 1</h1>
            <div class="subtitle">左側載重元件</div>
            <div id="weight1" class="weight-display s1-color">0.00</div>
            <div class="unit">kg</div>
            <div class="btn-group">
                <button class="btn-tare-s1" onclick="tare1()">歸零 S1</button>
            </div>
            <details class="calibration-panel">
                <summary>感測器 1 校正</summary>
                <div class="calibration-content">

                    <h4 class="cal-section-title">絕對歸零</h4>
                    <p class="calibration-note">感測器無負載時才能執行</p>
                    <button class="btn-cal" onclick="setZero(1, event)">設定零點</button>

                    <h4 class="cal-section-title">倍率校正</h4>
                    <p class="calibration-note">💡 請先在無負載時按「設定零點」，再放上已知重量</p>
                    <div class="cal-row">
                        <input type="number" id="calWeight1" class="cal-weight-input"
                               step="0.001" min="0.001" max="200" placeholder="已知重量 kg">
                        <button class="btn-cal" onclick="calibrateScale(1, event)">校正倍率</button>
                    </div>
                    <p class="cal-status" id="calStatus1">--</p>
                </div>
            </details>
        </div>

        <!-- ── Sensor 2 panel ──────────────────────────────────────── -->
        <div class="card">
            <h1>感測器 2</h1>
            <div class="subtitle">右側載重元件</div>
            <div id="weight2" class="weight-display s2-color">0.00</div>
            <div class="unit">kg</div>
            <div class="btn-group">
                <button class="btn-tare-s2" onclick="tare2()">歸零 S2</button>
            </div>
            <details class="calibration-panel">
                <summary>感測器 2 校正</summary>
                <div class="calibration-content">

                    <h4 class="cal-section-title">絕對歸零</h4>
                    <p class="calibration-note">感測器無負載時才能執行</p>
                    <button class="btn-cal" onclick="setZero(2, event)">設定零點</button>

                    <h4 class="cal-section-title">倍率校正</h4>
                    <p class="calibration-note">💡 請先在無負載時按「設定零點」，再放上已知重量</p>
                    <div class="cal-row">
                        <input type="number" id="calWeight2" class="cal-weight-input"
                               step="0.001" min="0.001" max="200" placeholder="已知重量 kg">
                        <button class="btn-cal" onclick="calibrateScale(2, event)">校正倍率</button>
                    </div>
                    <p class="cal-status" id="calStatus2">--</p>
                </div>
            </details>
        </div>

        <!-- ── Total panel ─────────────────────────────────────────── -->
        <div class="card">
            <h1>總重量</h1>
            <div class="subtitle">蜂箱 (S1 + S2)</div>
            <div id="weightTotal" class="weight-display tot-color">0.00</div>
            <div class="unit">kg</div>
            <div class="chart-container">
                <canvas id="chartTotal"></canvas>
            </div>
            <div class="btn-group">
                <button class="btn-record" onclick="addRecord()">記錄資料</button>
                <button class="btn-sched" onclick="syncSheets()">同步至 Sheets</button>
            </div>

            <div class="table-container">
                <h3 id="recordsTitle" style="color:#555; font-size:15px;">近期紀錄</h3>
                <div class="table-scroll">
                    <table>
                        <thead>
                            <tr>
                                <th>#</th>
                                <th>日期</th>
                                <th>時間</th>
                                <th>S1 (kg)</th>
                                <th>S2 (kg)</th>
                                <th>Total (kg)</th>
                                <th>刪除</th>
                            </tr>
                        </thead>
                        <tbody id="recordBody"></tbody>
                    </table>
                </div>
                <button class="btn-clear-all" onclick="clearAll()">清空所有紀錄</button>
                {{DEV_FACTORY_BUTTONS}}
            </div>
        </div>

        <!-- ── Wake-up Schedule panel ──────────────────────────────── -->
        <div class="card">
            <h1>喚醒排程</h1>
            <div class="subtitle">每日深度睡眠喚醒時間（上限 10 組）</div>

            <div style="display:flex; gap:8px; justify-content:center; align-items:center; margin:16px 0;">
                <select id="schedHour" style="padding:8px; border-radius:6px; border:1px solid #ccc; font-size:15px;">
                </select>
                <span style="font-size:18px; font-weight:bold;">:</span>
                <select id="schedMin" style="padding:8px; border-radius:6px; border:1px solid #ccc; font-size:15px;">
                </select>
                <button class="btn-sched" onclick="addSchedule()">新增</button>
            </div>

            <div id="schedList" style="text-align:left; max-width:260px; margin:0 auto;"></div>
            <button class="btn-clear-all" onclick="clearSchedule()" style="margin-top:12px;">清空所有排程</button>
        </div>

    </div><!-- .grid -->

    <script src="/chartjs"></script>
    <script>
        const PAGE_RENDERED_DEV = {{PAGE_RENDERED_DEV}};
        const MAX_PTS = 30;
        const MAX_VISIBLE_RECORDS = 20;
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

        /* ── Combined polling (1 s): sensors + wifi status in one request ── */
        setInterval(function() {
            fetch('/tick').then(r => r.json()).then(data => {
                const v1    = parseFloat(data.s1) || 0;
                const v2    = parseFloat(data.s2) || 0;
                const total = parseFloat((v1 + v2).toFixed(2));

                document.getElementById('weight1').innerText     = v1.toFixed(2);
                document.getElementById('weight2').innerText     = v2.toFixed(2);
                document.getElementById('weightTotal').innerText = total.toFixed(2);

                if (chartTotal) pushChart(chartTotal, total);

                // Update heartbeat indicator from the same response (when WIFI_CONFIG_ENABLED)
                if (data.wifi) {
                    const dot = document.getElementById('netDot');
                    if (dot) {
                        dot.className = 'dot dot-' + data.wifi.status;
                        document.getElementById('netSsid').innerText =
                            data.wifi.current_ssid ||
                            (data.wifi.target_ssid ? '(connecting to ' + data.wifi.target_ssid + ')' : 'Not connected');
                        document.getElementById('netIp').innerText =
                            data.wifi.ip ? '\u00b7 ' + data.wifi.ip : '';
                    }
                }
            }).catch(() => {});
        }, 1000);

        /* ── Clock ──────────────────────────────────────────────────── */
        setInterval(function() {
            document.getElementById('clock').innerText =
                '系統時間：' + new Date().toLocaleTimeString('en-US', { hour12: false });
        }, 1000);

        /* ── Tare ───────────────────────────────────────────────────── */
        function tare1() { fetch('/tare1'); }
        function tare2() { fetch('/tare2'); }

        /* ── 絕對歸零 ──────────────────────────────────────────────── */
        function setZero(sensor, event) {
            const btn = event.target;
            openConfirm(
                '絕對歸零 — 感測器 ' + sensor,
                '確認感測器 ' + sensor + ' 完全沒有負載。此動作會覆寫 NVS 內的零點偏移。',
                () => {
                    const orig = btn.innerHTML;
                    btn.innerHTML = '校正中...';
                    btn.disabled = true;
                    fetch('/set-zero' + sensor)
                        .then(r => r.text())
                        .then(() => {
                            alert('感測器 ' + sensor + ' 零點已儲存');
                            btn.innerHTML = orig;
                            btn.disabled = false;
                        })
                        .catch(() => {
                            alert('校正失敗 — 檢查連線');
                            btn.innerHTML = orig;
                            btn.disabled = false;
                        });
                }
            );
        }

        /* ── 倍率校正 ──────────────────────────────────────────────── */
        function calibrateScale(sensor, event) {
            const input  = document.getElementById('calWeight' + sensor);
            const status = document.getElementById('calStatus' + sensor);
            const btn    = event.target;
            const w      = parseFloat(input.value);

            if (!w || w <= 0) {
                status.className = 'cal-status error';
                status.innerText = '請輸入有效的重量';
                return;
            }

            openConfirm(
                '倍率校正 — 感測器 ' + sensor,
                '以 ' + w + ' kg 為基準校正感測器 ' + sensor + ' 的倍率。請確認重物已放在感測器上。此動作會覆寫 NVS 內的校正倍率。',
                () => {
                    const orig = btn.innerText;
                    btn.disabled = true;
                    btn.innerText = '校正中...';
                    status.className = 'cal-status';
                    status.innerText = '校正中...（取樣 10 筆）';

                    fetch('/calibrate-scale' + sensor + '?w=' + encodeURIComponent(w))
                        .then(r => r.json().then(d => ({ ok: r.ok, body: d })))
                        .then(({ ok, body }) => {
                            if (ok && body.factor) {
                                status.className = 'cal-status success';
                                status.innerText = '✓ 已套用校正值 (' + body.factor.toFixed(2) + ')';
                            } else {
                                status.className = 'cal-status error';
                                status.innerText = '✗ ' + (body.error || '校正失敗');
                            }
                        })
                        .catch(err => {
                            status.className = 'cal-status error';
                            status.innerText = '✗ 網路錯誤 — ' + err.message;
                        })
                        .finally(() => {
                            btn.disabled = false;
                            btn.innerText = orig;
                        });
                }
            );
        }

        /* ── Records ────────────────────────────────────────────────── */
        function renderTable(data) {
            const records = typeof data === 'string' ? JSON.parse(data) : data;
            const total = records.length;
            const visible = records.slice(0, MAX_VISIBLE_RECORDS);
            const tbody = document.getElementById('recordBody');
            tbody.innerHTML = '';
            visible.forEach((item, i) => {
                const s1 = parseFloat(item.sensor1) || 0;
                const s2 = parseFloat(item.sensor2) || 0;
                const sumStr = (s1 + s2).toFixed(3);
                tbody.innerHTML += `<tr>
                    <td>${i + 1}</td>
                    <td>${item.date || '-'}</td>
                    <td>${item.time}</td>
                    <td>${s1.toFixed(3)}</td>
                    <td>${s2.toFixed(3)}</td>
                    <td>${sumStr}</td>
                    <td><button class="btn-del" onclick="deleteRecord(${i})">&#x2715;</button></td>
                </tr>`;
            });

            // Update header to show how many records are shown vs how many exist
            const titleEl = document.getElementById('recordsTitle');
            if (titleEl) {
                if (total === 0) {
                    titleEl.innerText = '近期紀錄（無）';
                } else if (total <= visible.length) {
                    titleEl.innerText = `近期紀錄（${total}）`;
                } else {
                    titleEl.innerText = `近期紀錄（最新 ${visible.length} / 共 ${total}）`;
                }
            }
        }

        function fetchRecords(retry) {
            retry = retry || 0;
            fetch('/get-records')
                .then(r => {
                    if (!r.ok) throw new Error('HTTP ' + r.status);
                    return r.json();
                })
                .then(renderTable)
                .catch(err => {
                    console.warn('[fetchRecords] attempt ' + (retry + 1) + ' failed:', err);
                    if (retry < 2) {
                        setTimeout(function() { fetchRecords(retry + 1); }, 500 * (retry + 1));
                    } else {
                        console.error('[fetchRecords] giving up after 3 attempts');
                        const tbody = document.getElementById('recordBody');
                        if (tbody && tbody.children.length === 0) {
                            tbody.innerHTML = '<tr><td colspan="7" style="color:#e74c3c; padding:12px;">Failed to load records — ' + err.message + ' (after 3 retries)</td></tr>';
                        }
                    }
                });
        }

        function addRecord() {
            const now = new Date();
            const dateStr = now.toISOString().slice(0, 10);
            const timeStr = now.toLocaleTimeString('en-US', { hour12: false });
            const s1Val = document.getElementById('weight1').innerText;
            const s2Val = document.getElementById('weight2').innerText;
            fetch('/add-record?t=' + encodeURIComponent(timeStr)
                + '&d=' + dateStr + '&s1=' + s1Val + '&s2=' + s2Val)
                .then(r => r.json())
                .then(renderTable)
                .catch(err => console.error('[addRecord] failed:', err));
        }

        function deleteRecord(index) {
            fetch('/del-record?i=' + index)
                .then(r => r.json())
                .then(renderTable)
                .catch(err => console.error('[deleteRecord] failed:', err));
        }

        function clearAll() {
            if (!confirm('清空所有歷史紀錄？此動作無法復原。')) return;
            fetch('/clear-records')
                .then(r => r.json())
                .then(renderTable)
                .catch(err => console.error('Clear failed', err));
        }

        function factoryReset(full) {
            const title = full ? 'FULL Factory Reset' : 'Factory Reset';
            const msg = full
                ? '⚠️ 此動作會清除所有 records、重置 ID、清除 WiFi 認證與排程。校正值會保留。'
                : '⚠️ 此動作會清除所有 records 與重置 ID。校正、WiFi、排程會保留。\n\n注意：若有啟用 Google Sheets 同步，請手動清除 sheet 對應 row 以避免 silent dedup loss。';
            openConfirm(title, msg, () => {
                fetch('/factory-reset' + (full ? '?full=1' : ''), { method: 'POST' })
                    .then(r => r.text())
                    .then(text => {
                        alert(text);
                        location.reload();
                    })
                    .catch(err => {
                        alert('Factory reset failed: ' + err);
                    });
            });
        }

        /* ── Google Sheets sync ─────────────────────────────────────── */
        function syncSheets() {
            const btn = event.target;
            btn.innerText = '同步中...';
            btn.disabled = true;
            fetch('/sync-sheets')
                .then(r => r.text())
                .then(msg => { alert(msg); btn.innerText = '同步至 Sheets'; btn.disabled = false; })
                .catch(() => { alert('同步失敗'); btn.innerText = '同步至 Sheets'; btn.disabled = false; });
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
            fetch('/get-schedule')
                .then(r => {
                    if (!r.ok) throw new Error('HTTP ' + r.status);
                    return r.json();
                })
                .then(renderSchedule)
                .catch(err => console.error('[fetchSchedule] failed:', err));
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
            fetch('/del-schedule?i=' + i)
                .then(r => r.json())
                .then(renderSchedule)
                .catch(err => console.error('[delSchedule] failed:', err));
        }

        function clearSchedule() {
            if (!confirm('刪除所有喚醒時間設定？')) return;
            fetch('/clear-schedule', { method: 'POST' })
                .then(r => r.json())
                .then(renderSchedule)
                .catch(err => console.error('Clear schedule failed', err));
        }

        /* ── Init ───────────────────────────────────────────────────── */
        window.onload = function() {
            populateSelects();

            if (typeof Chart !== 'undefined') {
                chartTotal = makeChart('chartTotal', '#27ae60');
            } else {
                console.error('Chart.js not loaded — upload LittleFS image');
            }

            // Stagger initial fetches to avoid TCP backlog congestion on ESP32
            // (single-threaded WebServer + lwIP backlog ~5 connections)
            setTimeout(fetchRecords,  100);
            setTimeout(fetchSchedule, 300);
            setTimeout(function() {
                const timestamp = Math.floor(Date.now() / 1000);
                fetch('/sync?t=' + timestamp)
                    .then(response => {
                        if (response.ok) {
                            setTimeout(fetchRecords, 1500);
                        } else {
                            console.warn('[/sync] HTTP ' + response.status);
                        }
                    })
                    .catch(err => console.error('[/sync] failed:', err));
            }, 600);

            setInterval(fetchRecords, 60000);
        };

        /* ── Reusable confirm modal ────────────────────────────────── */
        let __confirmCallback = null;
        function openConfirm(title, message, callback) {
            document.getElementById('confirmTitle').innerText = title;
            document.getElementById('confirmMessage').innerText = message;
            const input = document.getElementById('confirmInput');
            input.value = '';
            const okBtn = document.getElementById('confirmOkBtn');
            okBtn.disabled = true;
            input.oninput = () => {
                okBtn.disabled = input.value.trim() !== 'CONFIRM';
            };
            __confirmCallback = callback;
            document.getElementById('confirmModal').style.display = 'flex';
            input.focus();
        }
        function closeConfirm() {
            document.getElementById('confirmModal').style.display = 'none';
            __confirmCallback = null;
        }
        function acceptConfirm() {
            const cb = __confirmCallback;
            closeConfirm();
            if (cb) cb();
        }
    </script>
<!-- Reusable confirm modal -->
<div id="confirmModal" style="display:none; position:fixed; inset:0;
     background:rgba(0,0,0,0.5); z-index:1000;
     align-items:center; justify-content:center;">
  <div style="background:white; padding:24px; border-radius:8px;
       max-width:420px; width:90%; box-shadow:0 4px 20px rgba(0,0,0,0.3);">
    <h3 id="confirmTitle" style="margin:0 0 8px;">Confirm</h3>
    <p id="confirmMessage" style="margin:8px 0 16px; color:#444;"></p>
    <p style="font-size:13px; color:#666;">輸入 <code style="background:#eee; padding:2px 6px; border-radius:3px;">CONFIRM</code> 以啟用：</p>
    <input id="confirmInput" type="text"
           style="width:100%; padding:8px; border:1px solid #ccc; border-radius:4px;" />
    <div style="text-align:right; margin-top:16px;">
      <button onclick="closeConfirm()" style="margin-right:8px; padding:8px 16px;">取消</button>
      <button id="confirmOkBtn" disabled onclick="acceptConfirm()"
              style="padding:8px 16px; background:#c0392b; color:white;
                     border:none; border-radius:4px; cursor:pointer;">執行</button>
    </div>
  </div>
</div>
</body>
</html>
)rawliteral";
#if DEV_MODE_ENABLED
    bool dev = isDevMode();
    if (dev) {
        html.replace("{{DEV_BANNER}}",
            "<div style=\"background:#c0392b; color:white; padding:8px; "
            "text-align:center; font-weight:bold; font-size:14px;\">"
            "&#9888; DEVELOPER MODE &mdash; remember to disable before delivery (or just reboot)"
            "</div>");
        html.replace("{{DEV_FACTORY_BUTTONS}}",
            "<button class=\"btn-clear-all\" style=\"border:2px solid #c0392b; "
            "background:#7f1f1a;\" onclick=\"factoryReset(false)\">"
            "&#x1F527; Factory Reset</button>"
            "<button class=\"btn-clear-all\" style=\"border:2px solid #c0392b; "
            "background:#7f1f1a;\" onclick=\"factoryReset(true)\">"
            "&#x1F527; Factory Reset (FULL)</button>");
    } else {
        html.replace("{{DEV_BANNER}}", "");
        html.replace("{{DEV_FACTORY_BUTTONS}}", "");
    }
    html.replace("{{PAGE_RENDERED_DEV}}", dev ? "true" : "false");
#else
    html.replace("{{DEV_BANNER}}", "");
    html.replace("{{DEV_FACTORY_BUTTONS}}", "");
    html.replace("{{PAGE_RENDERED_DEV}}", "false");
#endif

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
  <title>WiFi 網路設定</title>
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
{{DEV_BANNER}}
  <a class="back" href="/">&larr; 返回主控台</a>
  <h1>WiFi 網路設定</h1>

  <div class="card">
    <h2>目前狀態</h2>
    <div class="row"><span class="k">狀態</span>
      <span><span id="curDot" class="dot dot-disconnected"></span><span id="curStatus">--</span></span></div>
    <div class="row"><span class="k">SSID</span><span id="curSsid">--</span></div>
    <div class="row"><span class="k">IP</span><span id="curIp">--</span></div>
    <div class="row"><span class="k">訊號強度</span><span id="curRssi">--</span></div>
  </div>

  <div class="card">
    <h2>可用網路 <button class="scan" onclick="doScan()">&#x21bb; 重新掃描</button></h2>
    <div id="scanList"><p style="color:#95a5a6; text-align:center;">點擊 &#x21bb; 重新掃描以搜尋網路</p></div>
  </div>

  <div class="card" id="formCard" style="display:none;">
    <h2>連線</h2>
    <div class="field">
      <label>SSID</label>
      <input id="fSsid" type="text" placeholder="網路名稱">
    </div>
    <div class="field" id="passField">
      <label>密碼</label>
      <div class="pwrow">
        <input id="fPass" type="password" placeholder="8-63 個字元">
        <button class="eye" type="button" onclick="togglePw()">&#128065;</button>
      </div>
    </div>
    <button id="connectBtn" class="primary" onclick="doConnect()">連線</button>
    <div id="msg"></div>
  </div>

<script>
let pollTimer = null;

function $(id) { return document.getElementById(id); }

function esc(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

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
  }).catch(() => ({ status: 'disconnected' }));
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
    el.innerHTML = '<p style="color:#e74c3c; text-align:center;">&#9888; 附近沒有可用網路</p>';
    $('formCard').style.display = 'block';
    $('fSsid').value = '';
    $('fSsid').focus();
    return;
  }
  let html = '';
  list.forEach((ap, i) => {
    const lock = (ap.enc === 'OPEN') ? '' : '&#128274;';
    html += '<div class="ap" data-i="' + i + '" onclick="selectAp(' + i + ')">'
         +    '<div><span class="ssid">' + esc(ap.ssid) + '</span> '
         +      '<span class="meta">' + lock + '</span></div>'
         +    '<div><span class="bars">' + rssiBars(ap.rssi) + '</span> '
         +      '<span class="meta">' + esc(String(ap.rssi)) + ' dBm</span></div>'
         +  '</div>';
  });
  html += '<div class="ap" onclick="selectOther()" style="color:#3498db;">'
       +    '<div>&#9656; 其他（隱藏網路）</div></div>';
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
  if (pollTimer) { clearInterval(pollTimer); pollTimer = null; }
  $('connectBtn').disabled = false;
  $('connectBtn').innerText = '連線';
  $('scanList').innerHTML = '<p style="color:#95a5a6; text-align:center;">掃描中...</p>';
  fetch('/network/scan').then(r => {
    if (r.status === 409) { show('msg', 'err', 'WiFi 忙碌，請稍後再試'); return null; }
    if (!r.ok) { show('msg', 'err', '掃描失敗 (' + r.status + ')'); return null; }
    return r.json();
  }).then(list => { if (list) renderScan(list); }).catch(() => {});
}

function startPolling() {
  if (pollTimer) return;
  let elapsed = 0;
  pollTimer = setInterval(() => {
    elapsed += 1;
    refreshStatus().then(s => {
      if (s.status === 'connected') {
        clearInterval(pollTimer); pollTimer = null;
        show('msg', 'ok', '\u2713 \u5df2\u9023\u7dda\u5230 ' + s.current_ssid + ' (' + s.ip + ')');
        $('connectBtn').disabled = false;
        $('connectBtn').innerText = '連線';
      } else if (s.status === 'failed' || elapsed >= 10) {
        clearInterval(pollTimer); pollTimer = null;
        show('msg', 'err', '\u2717 連線失敗 — 密碼錯誤或超出範圍');
        $('fPass').value = '';
        $('connectBtn').disabled = false;
        $('connectBtn').innerText = '連線';
      }
    });
  }, 1000);
}

function doConnect() {
  const ssid = $('fSsid').value.trim();
  const pass = $('fPass').value;
  if (!ssid) { show('msg', 'err', '請輸入 SSID'); return; }
  if ($('passField').style.display !== 'none' && pass.length > 0 &&
      (pass.length < 8 || pass.length > 63)) {
    show('msg', 'err', '密碼長度必須為 8-63 個字元'); return;
  }

  $('connectBtn').disabled = true;
  $('connectBtn').innerText = '連線中...';
  show('msg', 'info', '連線到 ' + ssid + '...');

  const body = 's=' + encodeURIComponent(ssid) + '&p=' + encodeURIComponent(pass);
  fetch('/network/save', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  }).then(r => {
    if (r.status === 202) { startPolling(); }
    else if (r.status === 409) {
      r.json().then(d => {
        show('msg', 'info', '連線到 ' + (d.current_ssid || '...'));
        startPolling();
      });
    } else if (r.status === 400) {
      r.json().then(d => {
        show('msg', 'err', d.error || '請求錯誤');
        $('connectBtn').disabled = false;
        $('connectBtn').innerText = '連線';
      });
    } else {
      show('msg', 'err', '伺服器錯誤 (' + r.status + ')');
      $('connectBtn').disabled = false;
      $('connectBtn').innerText = '連線';
    }
  }).catch(() => {
    show('msg', 'err', '網路錯誤 — 請檢查連線');
    $('connectBtn').disabled = false;
    $('connectBtn').innerText = '連線';
  });
}

window.onload = function() {
  refreshStatus().then(s => {
    if (s.status === 'connecting') {
      $('formCard').style.display = 'block';
      $('connectBtn').disabled = true;
      $('connectBtn').innerText = '連線到 ' + (s.target_ssid || '...');
      startPolling();
    }
  });
};

/* ── Reusable confirm modal ────────────────────────────────── */
let __confirmCallback = null;
function openConfirm(title, message, callback) {
    document.getElementById('confirmTitle').innerText = title;
    document.getElementById('confirmMessage').innerText = message;
    const input = document.getElementById('confirmInput');
    input.value = '';
    const okBtn = document.getElementById('confirmOkBtn');
    okBtn.disabled = true;
    input.oninput = () => {
        okBtn.disabled = input.value.trim() !== 'CONFIRM';
    };
    __confirmCallback = callback;
    document.getElementById('confirmModal').style.display = 'flex';
    input.focus();
}
function closeConfirm() {
    document.getElementById('confirmModal').style.display = 'none';
    __confirmCallback = null;
}
function acceptConfirm() {
    const cb = __confirmCallback;
    closeConfirm();
    if (cb) cb();
}

function clearWifiCreds() {
    openConfirm(
        'Clear WiFi credentials',
        '此動作會清除 NVS 內的 WiFi 認證，下次開機會 fallback 到 compile-time SSID。',
        () => {
            fetch('/network/clear', { method: 'POST' })
                .then(r => r.json())
                .then(data => {
                    alert('WiFi credentials cleared. Reboot to apply.');
                })
                .catch(err => alert('Clear failed: ' + err));
        }
    );
}
</script>
<!-- Reusable confirm modal -->
<div id="confirmModal" style="display:none; position:fixed; inset:0;
     background:rgba(0,0,0,0.5); z-index:1000;
     align-items:center; justify-content:center;">
  <div style="background:white; padding:24px; border-radius:8px;
       max-width:420px; width:90%; box-shadow:0 4px 20px rgba(0,0,0,0.3);">
    <h3 id="confirmTitle" style="margin:0 0 8px;">Confirm</h3>
    <p id="confirmMessage" style="margin:8px 0 16px; color:#444;"></p>
    <p style="font-size:13px; color:#666;">輸入 <code style="background:#eee; padding:2px 6px; border-radius:3px;">CONFIRM</code> 以啟用：</p>
    <input id="confirmInput" type="text"
           style="width:100%; padding:8px; border:1px solid #ccc; border-radius:4px;" />
    <div style="text-align:right; margin-top:16px;">
      <button onclick="closeConfirm()" style="margin-right:8px; padding:8px 16px;">取消</button>
      <button id="confirmOkBtn" disabled onclick="acceptConfirm()"
              style="padding:8px 16px; background:#c0392b; color:white;
                     border:none; border-radius:4px; cursor:pointer;">執行</button>
    </div>
  </div>
</div>
{{DEV_NETWORK_BUTTONS}}
</body>
</html>
)rawliteral";
#if DEV_MODE_ENABLED
    bool dev = isDevMode();
    if (dev) {
        html.replace("{{DEV_BANNER}}",
            "<div style=\"background:#c0392b; color:white; padding:8px; "
            "text-align:center; font-weight:bold; font-size:14px;\">"
            "&#9888; DEVELOPER MODE &mdash; remember to disable before delivery (or just reboot)"
            "</div>");
        html.replace("{{DEV_NETWORK_BUTTONS}}",
            "<div class=\"card\" style=\"border:2px solid #c0392b;\">"
            "<h2 style=\"color:#c0392b;\">&#x1F527; Developer</h2>"
            "<button class=\"primary\" style=\"background:#c0392b;\" "
            "onclick=\"clearWifiCreds()\">Clear WiFi credentials (NVS)</button>"
            "<p style=\"font-size:13px; color:#7f8c8d;\">"
            "Removes saved credentials so the device falls back to the compile-time SSID at next boot."
            "</p></div>");
    } else {
        html.replace("{{DEV_BANNER}}", "");
        html.replace("{{DEV_NETWORK_BUTTONS}}", "");
    }
#else
    html.replace("{{DEV_BANNER}}", "");
    html.replace("{{DEV_NETWORK_BUTTONS}}", "");
#endif

    return html;
}
#endif // WIFI_CONFIG_ENABLED
