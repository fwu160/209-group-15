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

  /* top row: demo button on the left, status dot pinned right */
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

  /* Demo button - outline only so it doesn't fight visually with the
     status indicator sitting right next to it. */
  .topbar button.secondary {
    background: #fff;
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 9px 18px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
  }
  .topbar button.secondary:hover { opacity: 0.9; }

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

  /* 3 meter cards in a row (voltage/current/power) */
  .meters {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
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

  /* stack meter cards on small screens instead of squashing them */
  @media (max-width: 560px) {
    .meters { grid-template-columns: 1fr; }
  }
</style>
</head>
<body>
<div class="wrap">

  <h1>Energy Monitor</h1>
  <p class="subtitle">Live readings from the ESP32 bridge</p>

  <div class="topbar">
    <!-- ===== DEMO MODE (markup) =====
         To remove demo mode: delete this <button>, the
         ".topbar button.secondary" CSS rule above (optional tidy-up,
         harmless to leave), and the whole "DEMO MODE" block down in
         <script> below. Nothing else needs to change. -->
    <button id="demoBtn" class="secondary">Demo data</button>
    <!-- ===== END DEMO MODE (markup) ===== -->

    <div class="status">
      <span class="dot" id="statusDot"></span>
      <span id="statusText">Connecting…</span>
    </div>
  </div>

  <!-- the 3 live readouts -->
  <div class="meters" id="meters">
    <div class="meter">
      <div class="label">Voltage</div>
      <div class="value" id="vVal">--<span class="unit">V</span></div>
    </div>
    <div class="meter">
      <div class="label">Current</div>
      <div class="value" id="iVal">--<span class="unit">A</span></div>
    </div>
    <div class="meter">
      <div class="label">Power</div>
      <div class="value" id="pVal">--<span class="unit">W</span></div>
    </div>
  </div>

  <div class="graph">
    <div class="label">Power over time</div>
    <canvas id="chart" width="720" height="180"></canvas>
  </div>

</div>

<script>
  // ---- grab all the elements we need up front ----
  const vVal = document.getElementById('vVal');
  const iVal = document.getElementById('iVal');
  const pVal = document.getElementById('pVal');
  const metersEl = document.getElementById('meters');
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const canvas = document.getElementById('chart');
  const ctx = canvas.getContext('2d');   // 2D drawing API for <canvas>

  let ws = null;            // the active WebSocket (or null if not connected)
  const history = [];       // recent power readings, oldest first - feeds the graph
  const MAX_POINTS = 60;    // how many samples the graph keeps on screen
  const MAX_POWER_W = 8;    // graph y-axis ceiling - device maxes at 7.5VA, with headroom

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
    if (typeof stopDemo === 'function') stopDemo();   // don't let demo data fight real data
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
      // Every message is a small JSON blob, e.g.
      // {"v":230.42,"i":2.351,"p":541.87,"t":123456,"c":1}
      // "c" is 1 if the ATmega's UART is actively sending, 0 if it's
      // gone quiet - the only source of truth for the status dot. Sent
      // on both new readings and a periodic heartbeat, so a dead link
      // still gets noticed even with no fresh data.
      try {
        const d = JSON.parse(evt.data);
        const live = !!d.c;
        setStatus(live, live ? 'Connected' : 'Disconnected');
        if (live) updateReading(d.v, d.i, d.p);
      } catch (e) {
        // Malformed/cut-off message - just ignore it, another one's coming soon.
      }
    };
  }

  // ---- pushes a new reading onto the page ----
  // Both the real WebSocket data and demo mode call this, so the
  // meters/graph behave the same either way.
  function updateReading(v, i, p) {
    // innerHTML (not textContent) since we need the <span class="unit"> back each time.
    // Formats match spec: voltage to 1dp, current as a whole number in mA, power to 2dp.
    vVal.innerHTML = v.toFixed(1) + '<span class="unit">V</span>';
    iVal.innerHTML = Math.round(i) + '<span class="unit">mA</span>';
    pVal.innerHTML = p.toFixed(2) + '<span class="unit">W</span>';

    history.push(p);
    if (history.length > MAX_POINTS) history.shift();   // drop oldest once the buffer's full
    drawChart();
  }

  // ---- draws the power-vs-time graph ----
  function drawChart() {
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0, 0, w, h);   // wipe last frame before redrawing

    // light horizontal gridlines, purely decorative
    ctx.strokeStyle = '#f0f0f0';
    ctx.lineWidth = 1;
    for (let gy = 0; gy <= h; gy += h / 4) {
      ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(w, gy); ctx.stroke();
    }

    if (history.length < 2) return;   // need at least 2 points for a line

    // axis labels: max power top-left, time span along the bottom
    ctx.fillStyle = '#767676';
    ctx.font = '11px -apple-system, sans-serif';
    ctx.textAlign = 'left';
    ctx.fillText(MAX_POWER_W + ' W', 6, 14);
    ctx.fillText('-60s', 6, h - 6);
    ctx.textAlign = 'right';
    ctx.fillText('now', w - 6, h - 6);
    ctx.textAlign = 'left';

    // plot each reading as a point and join them up
    ctx.beginPath();
    ctx.strokeStyle = '#2f6fed';
    ctx.lineWidth = 2;
    history.forEach((p, idx) => {
      // array index -> x position across the canvas, power value -> y
      // position (canvas y grows downward, so we flip it).
      const x = (idx / (MAX_POINTS - 1)) * w;
      const y = h - (p / MAX_POWER_W) * h;
      if (idx === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();   // actually paint the line
  }

  // ============================================================
  // DEMO MODE - fakes data so the page can be tested/styled without
  // any ESP32 hardware plugged in.
  //
  // TO REMOVE: delete this whole block (down to "END DEMO MODE") plus
  // the <button id="demoBtn"> up in the markup. The
  // `typeof stopDemo === 'function'` check in connect() will then just
  // silently do nothing, no other changes needed.
  // ============================================================
  const demoBtn = document.getElementById('demoBtn');
  let demoTimer = null;   // setInterval ID, so we can cancel it later

  function startDemo() {
    if (ws) { ws.close(); ws = null; }   // don't let real data fight fake data
    setStatus(true, 'Demo mode (synthetic data)');

    let t = 0;
    if (demoTimer) clearInterval(demoTimer);   // stop any previous run first

    // fires every 500ms until cleared. Math.sin gives a smooth wave so
    // it looks like a plausible live signal rather than random noise.
    demoTimer = setInterval(() => {
      t += 0.5;
      const v = 14 + Math.sin(t / 4) * 1.2;                       // stays in the 12.6-15.4V supply range
      const p = 4 + Math.sin(t / 2) * 2.4 + Math.random() * 0.4;  // stays under the 7.5VA max
      const i = (p / v) * 1000;   // mA, stays under the 595mA max
      updateReading(v, i, p);
    }, 500);
  }

  function stopDemo() {
    if (demoTimer) {
      clearInterval(demoTimer);
      demoTimer = null;
    }
  }

  demoBtn.addEventListener('click', startDemo);
  // ============================================================
  // END DEMO MODE
  // ============================================================

  connect();   // nothing to enter, no button to press - go straight to live data
</script>
</body>
</html>
)rawliteral";