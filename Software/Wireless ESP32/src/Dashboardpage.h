#pragma once

// The dashboard web page, served by the ESP32 (see TelemetryServer.cpp)
// The captive portal opens it as soon as a phone joins the WiFi
// The status dot shows if the ATmega is sending, not just if the browser is connected
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Energy Monitor</title>
<style>
  /* Colours, change these to reskin the page */
  :root {
    --bg: #ffffff;
    --border: #e0e0e0;
    --text: #1a1a1a;
    --muted: #767676;
    --accent: #2f6fed;
    --live: #2f9e44;
    --offline: #d9480f;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0;
    background: var(--bg);
    color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
    padding: 32px 24px 60px;
  }
  .wrap { max-width: 760px; margin: 0 auto; }   /* Centred, max width on big screens */

  h1 { font-size: 20px; font-weight: 600; margin: 0 0 4px; }
  .subtitle { font-size: 13px; color: var(--muted); margin: 0 0 24px; }

  /* Top bar with the status dot on the right */
  .topbar {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 14px 16px;
    margin-bottom: 24px;
  }
  .status {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 13px;
    color: var(--muted);
    margin-left: auto;
  }
  .dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--offline);   /* Red until JS sets it live */
  }
  .dot.live { background: var(--live); }

  /* Four reading cards in a row */
  .meters {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 14px;
    margin-bottom: 14px;
    transition: opacity 0.2s ease;
  }
  .meters.offline { opacity: 0.45; }   /* Fade old numbers while the ATmega is offline */
  .meter {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 18px;
  }
  .meter .label { font-size: 12px; color: var(--muted); margin-bottom: 6px; }
  .meter .value { font-size: 30px; font-weight: 600; }
  .meter .unit { font-size: 14px; color: var(--accent); margin-left: 4px; font-weight: 500; }

  /* Graph card */
  .graph {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 16px 18px 10px;
  }
  .graph-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    flex-wrap: wrap;
    gap: 8px;
    margin-bottom: 10px;
  }
  .graph .label { font-size: 12px; color: var(--muted); }
  canvas { width: 100%; height: 180px; display: block; }

  /* Buttons to pick what the graph shows */
  .tabs { display: flex; gap: 4px; flex-wrap: wrap; }
  .tabs button {
    font: inherit;
    font-size: 12px;
    color: var(--muted);
    background: none;
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 3px 10px;
    cursor: pointer;
  }
  .tabs button.active { color: #fff; background: var(--accent); border-color: var(--accent); }

  /* Cards go 2 x 2 on phones */
  @media (max-width: 640px) {
    .meters { grid-template-columns: repeat(2, 1fr); }
  }
</style>
</head>
<body>
<div class="wrap">

  <h1>Energy Monitor</h1>
  <p class="subtitle">Live readings from the ESP32 bridge</p>

  <div class="topbar">
    <div class="status">
      <span class="dot" id="statusDot"></span>
      <span id="statusText">Connecting…</span>
    </div>
  </div>

  <!-- The 4 live readings -->
  <div class="meters" id="meters">
    <div class="meter">
      <div class="label">Voltage (RMS)</div>
      <div class="value" id="vVal">--<span class="unit">V</span></div>
    </div>
    <div class="meter">
      <div class="label">Peak Current</div>
      <div class="value" id="iVal">--<span class="unit">mA</span></div>
    </div>
    <div class="meter">
      <div class="label">Power</div>
      <div class="value" id="pVal">--<span class="unit">W</span></div>
    </div>
    <div class="meter">
      <div class="label">Phase difference</div>
      <div class="value" id="phVal">--<span class="unit">&deg;</span></div>
    </div>
  </div>

  <div class="graph">
    <div class="graph-head">
      <div class="label" id="graphTitle">Power (W), last 60 s</div>
      <!-- Each data-key matches a field name in the JSON -->
      <div class="tabs" id="tabs">
        <button data-key="p" class="active">Power</button>
        <button data-key="v">Voltage</button>
        <button data-key="i">Current</button>
        <button data-key="ph">Phase</button>
      </div>
    </div>
    <canvas id="chart" width="720" height="180"></canvas>
  </div>

</div>

<script>
  // Grab the elements we need
  const vVal = document.getElementById('vVal');
  const iVal = document.getElementById('iVal');
  const pVal = document.getElementById('pVal');
  const phVal = document.getElementById('phVal');
  const metersEl = document.getElementById('meters');
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const graphTitle = document.getElementById('graphTitle');
  const canvas = document.getElementById('chart');
  const ctx = canvas.getContext('2d');   // For drawing on the canvas

  let ws = null;            // The WebSocket, null if not connected
  const history = [];       // Recent readings, newest first
  let lastT = null;         // Timestamp of the last frame we used
  const WINDOW_MS = 60000;  // Graph shows the last 60 s
  const X_STEP_MS = 10000;  // Gridline every 10 s
  const GAP_MS = 2000;      // Break the line if frames are missing for this long

  // Y axis settings for each graph, keys match the JSON fields
  const GRAPHS = {
    p:  { name: 'Power',         unit: 'W',  max: 10,   step: 2 },
    v:  { name: 'RMS Voltage',   unit: 'V',  max: 20,   step: 5 },
    i:  { name: 'Peak Current',  unit: 'mA', max: 1000, step: 200 },
    ph: { name: 'Phase',         unit: '°',  max: 90,   step: 15 },
  };
  let graphKey = 'p';       // What the graph shows now

  // Sets the dot, the text and fades the cards
  // Live means the ATmega is sending, not just that the WebSocket is open
  function setStatus(live, label) {
    statusDot.classList.toggle('live', live);
    statusText.textContent = label;
    metersEl.classList.toggle('offline', !live);
  }

  // Graph buttons: switch what's plotted and redraw
  document.querySelectorAll('#tabs button').forEach((btn) => {
    btn.addEventListener('click', () => {
      graphKey = btn.dataset.key;
      document.querySelectorAll('#tabs button').forEach((b) => b.classList.toggle('active', b === btn));
      drawChart();
    });
  });

  // Connects to the ESP32 that served this page
  function connect() {
    if (ws) ws.close();   // Close any old connection

    // WebSocket stays open so the ESP32 can push each reading to us
    ws = new WebSocket('ws://' + location.host + '/ws');

    ws.onopen = () => setStatus(false, 'Waiting for device…');
    ws.onclose = () => {
      setStatus(false, 'Reconnecting…');
      setTimeout(connect, 2000);   // Keep retrying
    };
    ws.onerror = () => setStatus(false, 'Connection error');
    ws.onmessage = (evt) => {
      // Example message: {"v":13.81,"i":654,"p":5.54,"ph":27.73,"t":123456,"c":1}
      // The "c" field is 1 if the ATmega is sending, also sent every 1 s as a heartbeat
      try {
        const d = JSON.parse(evt.data);
        const live = !!d.c;
        setStatus(live, live ? 'Connected' : 'Disconnected');
        // Heartbeats repeat the last frame, so only use frames with a new timestamp
        if (live && d.t !== lastT) {
          if (lastT !== null && d.t < lastT) history.length = 0;   // ESP32 restarted, start fresh
          lastT = d.t;
          updateReading(d);
        }
      } catch (e) {
        // Broken message, ignore it, another one is coming
      }
    };
  }

  // Shows a new reading, runs once per ATmega frame (2 per second)
  function updateReading(d) {
    // V, power and phase to 2dp, current in whole mA
    vVal.innerHTML = d.v.toFixed(2) + '<span class="unit">V</span>';
    iVal.innerHTML = Math.round(d.i) + '<span class="unit">mA</span>';
    pVal.innerHTML = d.p.toFixed(2) + '<span class="unit">W</span>';
    phVal.innerHTML = (d.ph || 0).toFixed(2) + '<span class="unit">&deg;</span>';

    // Save all 4 values so the graph can switch without losing history
    history.unshift({ t: d.t, v: d.v, i: d.i, p: d.p, ph: d.ph || 0 });   // Newest at the front
    // Drop points older than 60 s
    while (history.length && d.t - history[history.length - 1].t > WINDOW_MS) history.pop();
    drawChart();
  }

  // Draws the graph, newest on the left, x axis is real time from the ESP32 timestamps
  function drawChart() {
    const g = GRAPHS[graphKey];
    graphTitle.textContent = g.name + ' (' + g.unit + '), last 60 s';

    // Match the canvas to its size on screen so it stays sharp
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);

    // Plot area with room for the labels
    const padL = 36, padR = 10, padT = 8, padB = 22;
    const plotW = w - padL - padR, plotH = h - padT - padB;
    const xOf = (agoMs) => padL + (agoMs / WINDOW_MS) * plotW;
    const yOf = (val) => padT + plotH - (Math.min(Math.max(val, 0), g.max) / g.max) * plotH;

    ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif';
    ctx.fillStyle = '#9a9a9a';
    ctx.lineWidth = 1;

    // Y axis gridlines, very faint so they don't stand out
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    for (let val = 0; val <= g.max; val += g.step) {
      const y = Math.round(yOf(val)) + 0.5;   // Adding 0.5 keeps 1 px lines sharp
      ctx.strokeStyle = val === 0 ? 'rgba(0, 0, 0, 0.15)' : 'rgba(0, 0, 0, 0.05)';
      ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke();
      ctx.fillText(String(val), padL - 6, y);
    }

    // X axis gridline every 10 s, "now" on the left
    ctx.textBaseline = 'top';
    ctx.strokeStyle = 'rgba(0, 0, 0, 0.05)';
    for (let ago = 0; ago <= WINDOW_MS; ago += X_STEP_MS) {
      const x = Math.round(xOf(ago)) + 0.5;
      ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, padT + plotH); ctx.stroke();
      ctx.textAlign = ago === 0 ? 'left' : (ago === WINDOW_MS ? 'right' : 'center');
      ctx.fillText(ago === 0 ? 'now' : '-' + ago / 1000 + 's', x, padT + plotH + 6);
    }

    if (history.length === 0) return;

    // The line, from the newest point (left) to the oldest (right)
    const newest = history[0].t;
    ctx.strokeStyle = '#2f6fed';
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    ctx.beginPath();
    history.forEach((pt, idx) => {
      const x = xOf(newest - pt.t), y = yOf(pt[graphKey]);
      const gap = idx > 0 && history[idx - 1].t - pt.t > GAP_MS;   // Don't join across missing frames
      if (idx === 0 || gap) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();

    // Dot on the newest reading
    ctx.fillStyle = '#2f6fed';
    ctx.beginPath(); ctx.arc(xOf(0), yOf(history[0][graphKey]), 3, 0, 2 * Math.PI); ctx.fill();
  }

  window.addEventListener('resize', drawChart);   // Redraw if the window size changes
  drawChart();   // Draw the empty axes straight away
  connect();     // Go straight to live data
</script>
</body>
</html>
)rawliteral";