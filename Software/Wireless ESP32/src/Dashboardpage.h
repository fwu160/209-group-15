#pragma once

// The dashboard page itself, served directly by the ESP32. The AP's
// captive portal (see TelemetryServer.cpp) redirects a phone here the
// second it joins "EnergyMonitor", so it loads on its own - no address
// to type and nothing to press to connect.
//
// This is adapted from clean.html so there's nothing left to configure:
// it always talks back to whatever host actually served it
// (location.host), and the Connected/Disconnected indicator now reflects
// the ATmega's actual UART link (is it toggling around 3.3V or sitting
// idle at 0V), not just whether the browser's WebSocket happens to be
// open. That comes from the "c" field TelemetryServer sends with every
// message.
//
// This file is what actually gets served, so keep it as the source of
// truth if the dashboard gets redesigned again.
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Energy Monitor</title>
<style>
  /* Colour palette - change these to reskin the whole page */
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
  .wrap { max-width: 760px; margin: 0 auto; }   /* centers everything, caps width on big screens */

  h1 { font-size: 20px; font-weight: 600; margin: 0 0 4px; }
  .subtitle { font-size: 13px; color: var(--muted); margin: 0 0 24px; }

  /* top row: status dot pinned right */
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

  /* status dot + text, pushed to the right edge via margin-left: auto */
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
    background: var(--offline);   /* red by default, JS flips to green when live */
  }
  .dot.live { background: var(--live); }

  /* 4 meter cards in a row (voltage/current/power/phase) */
  .meters {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 14px;
    margin-bottom: 14px;
    transition: opacity 0.2s ease;
  }
  /* Fades the last-known numbers while the ATmega's offline, so it's
     obvious they're stale and not live. */
  .meters.offline { opacity: 0.45; }
  .meter {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 18px;
  }
  .meter .label { font-size: 12px; color: var(--muted); margin-bottom: 6px; }
  .meter .value { font-size: 30px; font-weight: 600; }
  .meter .unit { font-size: 14px; color: var(--accent); margin-left: 4px; font-weight: 500; }

  /* power-vs-time graph card */
  .graph {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 16px 18px 10px;
  }
  .graph .label { font-size: 12px; color: var(--muted); margin-bottom: 10px; }
  canvas { width: 100%; height: 180px; display: block; }

  /* 2 x 2 grid on small screens instead of squashing 4 cards into one row */
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

  <!-- the 4 live readouts -->
  <div class="meters" id="meters">
    <div class="meter">
      <div class="label">Voltage</div>
      <div class="value" id="vVal">--<span class="unit">V</span></div>
    </div>
    <div class="meter">
      <div class="label">Current</div>
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
    <div class="label">Power, last 60 s</div>
    <canvas id="chart" width="720" height="180"></canvas>
  </div>

</div>

<script>
  // ---- grab all the elements we need up front ----
  const vVal = document.getElementById('vVal');
  const iVal = document.getElementById('iVal');
  const pVal = document.getElementById('pVal');
  const phVal = document.getElementById('phVal');
  const metersEl = document.getElementById('meters');
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const canvas = document.getElementById('chart');
  const ctx = canvas.getContext('2d');   // 2D drawing API for <canvas>

  let ws = null;            // the active WebSocket (or null if not connected)
  const history = [];       // Recent readings as {t, p}, newest first
  let lastT = null;         // ESP32 timestamp of the last frame we plotted
  const WINDOW_MS = 60000;  // Graph shows the last 60 s
  const X_STEP_MS = 10000;  // Gridline every 10 s
  const GAP_MS = 2000;      // Break the line if frames go missing for this long
  const MAX_POWER_W = 10;   // Y axis top, max load is ~9 W with a +10% source
  const Y_STEP_W = 2;       // Gridline every 2 W

  // Updates the dot + text + dims the meters in one call instead of
  // repeating 3 lines everywhere. `live` means "ATmega is actually
  // sending UART frames", not just "WebSocket is open".
  function setStatus(live, label) {
    statusDot.classList.toggle('live', live);
    statusText.textContent = label;
    metersEl.classList.toggle('offline', !live);
  }

  // ---- connecting to the ESP32 over WebSocket ----
  // Page is always served by the ESP32 itself, so it always talks back
  // to whatever host served it (location.host) - nothing to configure.
  function connect() {
    if (ws) ws.close();   // close any old connection first

    // ws:// is basically http:// but the connection stays open, so the
    // ESP32 can keep pushing readings instead of us polling for one.
    ws = new WebSocket('ws://' + location.host + '/ws');

    // event handlers the browser calls automatically - we never call these ourselves
    ws.onopen = () => setStatus(false, 'Waiting for device…');
    ws.onclose = () => {
      setStatus(false, 'Reconnecting…');
      setTimeout(connect, 2000);   // keep retrying
    };
    ws.onerror = () => setStatus(false, 'Connection error');
    ws.onmessage = (evt) => {
      // Every message is a small JSON blob
      // Example: {"v":13.81,"i":654,"p":5.54,"ph":27.73,"t":123456,"c":1}
      // "ph" is the phase difference between voltage and current, in degrees.
      // "c" is 1 if the ATmega's UART is actively sending, 0 if it's
      // gone quiet - the only source of truth for the status dot. Sent
      // on both new readings and a periodic heartbeat, so a dead link
      // still gets noticed even with no fresh data.
      try {
        const d = JSON.parse(evt.data);
        const live = !!d.c;
        setStatus(live, live ? 'Connected' : 'Disconnected');
        // Heartbeats repeat the last frame, so only use frames with a new timestamp
        if (live && d.t !== lastT) {
          if (lastT !== null && d.t < lastT) history.length = 0;   // ESP32 restarted, start fresh
          lastT = d.t;
          updateReading(d.v, d.i, d.p, d.ph, d.t);
        }
      } catch (e) {
        // Malformed/cut-off message - just ignore it, another one's coming soon.
      }
    };
  }

  // ---- pushes a new reading onto the page ----
  // Called once per new ATmega frame (2 per second).
  function updateReading(v, i, p, ph, t) {
    // innerHTML (not textContent) since we need the <span class="unit"> back each time.
    // V, power and phase to 2dp, current in whole mA
    // Phase shows 0 if the ATmega didn't send one
    vVal.innerHTML = v.toFixed(2) + '<span class="unit">V</span>';
    iVal.innerHTML = Math.round(i) + '<span class="unit">mA</span>';
    pVal.innerHTML = p.toFixed(2) + '<span class="unit">W</span>';
    phVal.innerHTML = (ph || 0).toFixed(2) + '<span class="unit">&deg;</span>';

    history.unshift({ t: t, p: p });   // Newest goes at the front, drawn on the left
    // Drop points that have scrolled off the right edge
    while (history.length && t - history[history.length - 1].t > WINDOW_MS) history.pop();
    drawChart();
  }

  // ---- draws the power-vs-time graph ----
  // Newest reading on the left, older ones move right. X is real time from the ESP32 timestamps.
  function drawChart() {
    // Match the canvas to its size on screen so text and lines stay sharp
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);

    // Plot area, leaving room for the axis labels
    const padL = 36, padR = 10, padT = 8, padB = 22;
    const plotW = w - padL - padR, plotH = h - padT - padB;
    const xOf = (agoMs) => padL + (agoMs / WINDOW_MS) * plotW;
    const yOf = (watts) => padT + plotH - (Math.min(Math.max(watts, 0), MAX_POWER_W) / MAX_POWER_W) * plotH;

    ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif';
    ctx.fillStyle = '#767676';
    ctx.lineWidth = 1;

    // Y axis: gridline and label every 2 W
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    for (let watts = 0; watts <= MAX_POWER_W; watts += Y_STEP_W) {
      const y = Math.round(yOf(watts)) + 0.5;   // +0.5 keeps 1 px lines crisp
      ctx.strokeStyle = watts === 0 ? '#c8c8c8' : '#eeeeee';
      ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke();
      ctx.fillText(watts === MAX_POWER_W ? watts + ' W' : String(watts), padL - 6, y);
    }

    // X axis: gridline every 10 s, "now" on the left
    ctx.textBaseline = 'top';
    for (let ago = 0; ago <= WINDOW_MS; ago += X_STEP_MS) {
      const x = Math.round(xOf(ago)) + 0.5;
      ctx.strokeStyle = '#eeeeee';
      ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, padT + plotH); ctx.stroke();
      ctx.textAlign = ago === 0 ? 'left' : (ago === WINDOW_MS ? 'right' : 'center');
      ctx.fillText(ago === 0 ? 'now' : '-' + ago / 1000 + 's', x, padT + plotH + 6);
    }

    if (history.length === 0) return;

    // The power line, from the newest point (left) to the oldest (right)
    const newest = history[0].t;
    ctx.strokeStyle = '#2f6fed';
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    ctx.beginPath();
    history.forEach((pt, idx) => {
      const x = xOf(newest - pt.t), y = yOf(pt.p);
      const gap = idx > 0 && history[idx - 1].t - pt.t > GAP_MS;   // Don't join across missing frames
      if (idx === 0 || gap) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();

    // Dot on the newest reading
    ctx.fillStyle = '#2f6fed';
    ctx.beginPath(); ctx.arc(xOf(0), yOf(history[0].p), 3, 0, 2 * Math.PI); ctx.fill();
  }

  window.addEventListener('resize', drawChart);   // Redraw sharp if the window size changes
  drawChart();   // Draw the empty axes straight away
  connect();   // nothing to enter, no button to press - go straight to live data
</script>
</body>
</html>
)rawliteral";