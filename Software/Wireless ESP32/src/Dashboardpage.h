#pragma once

// The dashboard page, served directly by the ESP32 - this is what makes the
// captive portal actually useful (redirecting a phone to "/" only helps if
// there's a real page waiting there), and it also means phones/laptops can
// just load it straight from the ESP32 instead of needing the HTML file
// copied onto them by hand.
//
// Content is clean.html, with one small addition carried over from the
// previous version: the "ESP32 address" box defaults to whatever host/IP
// actually served the page (see the added line just after the element
// references below), instead of a hardcoded "192.168.4.1" that might not
// match if you're using the energymonitor.local name or the IP ever changes.
//
// If you redesign the dashboard again, this file is what actually gets
// served - keep it as the source of truth, or re-sync it after editing
// your local copy.
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Energy Monitor</title>
<style>
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
  .wrap { max-width: 760px; margin: 0 auto; }

  h1 { font-size: 20px; font-weight: 600; margin: 0 0 4px; }
  .subtitle { font-size: 13px; color: var(--muted); margin: 0 0 24px; }

  .connect {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 14px 16px;
    margin-bottom: 24px;
  }
  .connect label { font-size: 13px; color: var(--muted); }
  .connect input {
    flex: 1;
    min-width: 180px;
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 8px 10px;
    font-size: 14px;
    color: var(--text);
  }
  .connect input:focus { outline: none; border-color: var(--accent); }
  .connect button {
    background: var(--accent);
    color: #fff;
    border: none;
    border-radius: 6px;
    padding: 9px 18px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
  }
  .connect button:hover { opacity: 0.9; }

  /* Demo button reuses the same shape as Connect but stays neutral
     (border only, no fill) so it doesn't compete with it visually. */
  .connect button.secondary {
    background: #fff;
    color: var(--text);
    border: 1px solid var(--border);
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
    background: var(--offline);
  }
  .dot.live { background: var(--live); }

  .meters {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 14px;
    margin-bottom: 14px;
  }
  .meter {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 18px;
  }
  .meter .label { font-size: 12px; color: var(--muted); margin-bottom: 6px; }
  .meter .value { font-size: 30px; font-weight: 600; }
  .meter .unit { font-size: 14px; color: var(--accent); margin-left: 4px; font-weight: 500; }

  .graph {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 16px 18px 10px;
  }
  .graph .label { font-size: 12px; color: var(--muted); margin-bottom: 10px; }
  canvas { width: 100%; height: 180px; display: block; }

  @media (max-width: 560px) {
    .meters { grid-template-columns: 1fr; }
  }
</style>
</head>
<body>
<div class="wrap">

  <h1>Energy Monitor</h1>
  <p class="subtitle">Live readings from the ESP32 bridge</p>

  <div class="connect">
    <label for="hostInput">ESP32 address</label>
    <input id="hostInput" type="text" value="192.168.4.1" spellcheck="false">
    <button id="connectBtn">Connect</button>

    <!-- ===== DEMO MODE (markup) =====
         To remove demo mode: delete this <button>, the ".secondary"
         CSS rule above (optional tidy-up, harmless to leave), and the
         whole "DEMO MODE" block down in <script> below. Nothing else
         needs to change. -->
    <button id="demoBtn" class="secondary">Demo data</button>
    <!-- ===== END DEMO MODE (markup) ===== -->

    <div class="status">
      <span class="dot" id="statusDot"></span>
      <span id="statusText">Offline</span>
    </div>
  </div>

  <div class="meters">
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
  // ---- element references ----
  // Grabbing all of these once up front (instead of re-querying the DOM
  // every time we need them) is just good practice — cheaper, and makes
  // it obvious at a glance which elements this page actually touches.
  const vVal = document.getElementById('vVal');
  const iVal = document.getElementById('iVal');
  const pVal = document.getElementById('pVal');
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const hostInput = document.getElementById('hostInput');
  const connectBtn = document.getElementById('connectBtn');
  const canvas = document.getElementById('chart');
  const ctx = canvas.getContext('2d');   // the 2D drawing API for <canvas>

  // Page is now served by the ESP32 itself, so whatever host/IP actually
  // got us here is guaranteed to be reachable - use that instead of the
  // hardcoded fallback above.
  if (location.hostname) hostInput.value = location.hostname;

  let ws = null;            // holds the active WebSocket connection (or null if none)
  const history = [];       // recent power readings, oldest first — feeds the graph
  const MAX_POINTS = 60;    // how many samples the graph keeps on screen at once
  const MAX_POWER_W = 8;    // graph y-axis ceiling - device is rated 7.5VA max, with a bit of headroom

  // Small helper so "update the status dot + text together" is one call
  // instead of two lines repeated in four different places below.
  function setStatus(live, label) {
    statusDot.classList.toggle('live', live);   // toggles the CSS class that turns the dot green
    statusText.textContent = label;
  }

  // ---- connecting to the ESP32 ----
  function connect() {
    // If demo mode (below) is running, stop it — we don't want synthetic
    // data and real data both trying to update the page at once.
    // typeof check makes this safe even if you've deleted the demo block
    // entirely: calling a function that doesn't exist would normally
    // throw an error, but `typeof missingName === 'function'` just
    // quietly evaluates to false instead of crashing.
    if (typeof stopDemo === 'function') stopDemo();

    const host = hostInput.value.trim();
    if (!host) return;
    if (ws) ws.close();   // close any previous connection before opening a new one

    setStatus(false, 'Connecting…');

    // 'ws://' is the WebSocket equivalent of 'http://' — same idea, but
    // the connection stays open so the ESP32 can keep pushing new
    // readings to us, instead of us having to ask for one every time.
    ws = new WebSocket('ws://' + host + '/ws');

    // These four are "event handlers" — functions the browser calls
    // automatically when something happens on the socket. We don't call
    // them ourselves anywhere.
    ws.onopen = () => setStatus(true, 'Connected');
    ws.onclose = () => {
      setStatus(false, 'Disconnected — retrying');
      setTimeout(connect, 2000);   // keep trying to reconnect automatically
    };
    ws.onerror = () => setStatus(false, 'Connection error');
    ws.onmessage = (evt) => {
      // Every message from the ESP32 is a small JSON string, e.g.
      // {"v":230.42,"i":2.351,"p":541.87,"t":123456}
      try {
        const d = JSON.parse(evt.data);
        updateReading(d.v, d.i, d.p);
      } catch (e) {
        // If a message arrives malformed/cut off, just ignore it rather
        // than letting the page crash — the next one will be along soon.
      }
    };
  }

  // ---- updating the page with a new reading ----
  // Both the real WebSocket data and demo mode (below) call this same
  // function, so the meters/graph behave identically either way.
  function updateReading(v, i, p) {
    // innerHTML (not textContent) here because we're re-inserting the
    // little <span class="unit"> alongside the number each time.
    // Formats match the required spec: voltage XX.X, current is a
    // whole number in mA, power X.XX
    vVal.innerHTML = v.toFixed(1) + '<span class="unit">V</span>';
    iVal.innerHTML = Math.round(i) + '<span class="unit">mA</span>';
    pVal.innerHTML = p.toFixed(2) + '<span class="unit">W</span>';

    history.push(p);
    if (history.length > MAX_POINTS) history.shift();   // drop the oldest point once we're full
    drawChart();
  }

  // ---- drawing the power graph ----
  function drawChart() {
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0, 0, w, h);   // wipe the previous frame before redrawing

    // light horizontal grid lines, purely decorative
    ctx.strokeStyle = '#f0f0f0';
    ctx.lineWidth = 1;
    for (let gy = 0; gy <= h; gy += h / 4) {
      ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(w, gy); ctx.stroke();
    }

    if (history.length < 2) return;   // need at least 2 points to draw a line

    // Basic axis labels: y-axis max at top-left, time span at the bottom
    ctx.fillStyle = '#767676';
    ctx.font = '11px -apple-system, sans-serif';
    ctx.textAlign = 'left';
    ctx.fillText(MAX_POWER_W + ' W', 6, 14);
    ctx.fillText('-60s', 6, h - 6);
    ctx.textAlign = 'right';
    ctx.fillText('now', w - 6, h - 6);
    ctx.textAlign = 'left';

    ctx.beginPath();
    ctx.strokeStyle = '#2f6fed';
    ctx.lineWidth = 2;
    history.forEach((p, idx) => {
      // Map each reading's array index -> an x position across the canvas,
      // and its power value -> a y position (canvas y grows downward, so
      // we subtract to make higher power draw higher up).
      const x = (idx / (MAX_POINTS - 1)) * w;
      const y = h - (p / MAX_POWER_W) * h;
      if (idx === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();   // actually paints the line we just plotted
  }

  connectBtn.addEventListener('click', connect);
  hostInput.addEventListener('keydown', (e) => { if (e.key === 'Enter') connect(); });

  // ============================================================
  // DEMO MODE — generates fake data so you can test/style this page
  // without any ESP32 hardware connected.
  //
  // TO REMOVE: delete this entire block (down to "END DEMO MODE"),
  // delete the <button id="demoBtn"> in the HTML above (and its
  // ".secondary" CSS rule, if you want to tidy up). The
  // `typeof stopDemo === 'function'` guard in connect() will then just
  // silently do nothing — no other changes needed.
  // ============================================================
  const demoBtn = document.getElementById('demoBtn');
  let demoTimer = null;   // holds the setInterval ID so we can cancel it later

  function startDemo() {
    if (ws) { ws.close(); ws = null; }   // don't let real data fight with fake data
    setStatus(true, 'Demo mode (synthetic data)');

    let t = 0;
    if (demoTimer) clearInterval(demoTimer);   // stop any previous demo run first

    // setInterval repeatedly calls this function every 500ms until we
    // clearInterval() it. Math.sin() generates a smooth wave so the
    // numbers look like a plausible live signal instead of random noise.
    demoTimer = setInterval(() => {
      t += 0.5;
      const v = 14 + Math.sin(t / 4) * 1.2;              // stays within the 12.6-15.4V supply range
      const p = 4 + Math.sin(t / 2) * 2.4 + Math.random() * 0.4;  // stays under the 7.5VA max
      const i = (p / v) * 1000;   // current in mA, stays under the 595mA max
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
</script>
</body>
</html>
)rawliteral";