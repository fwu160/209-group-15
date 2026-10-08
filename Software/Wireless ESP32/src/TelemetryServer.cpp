#include "TelemetryServer.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include "Dashboardpage.h"

// Turns a Reading into the JSON string the dashboard's JS expects, e.g.
// {"v":14.5,"i":300,"p":6.43,"ph":36.9,"t":123456,"c":1}
// "ph" = phase difference between voltage and current, in degrees.
// "c" = 1 if the ATmega's UART link is currently up, 0 if it's gone quiet.
String TelemetryServer::toJson(const Reading& r, bool connected) {
  char json[144];
  snprintf(json, sizeof(json),
    "{\"v\":%.1f,\"i\":%.0f,\"p\":%.2f,\"ph\":%.1f,\"t\":%lu,\"c\":%d}",
    r.voltage, r.current, r.power, r.phase, r.millisAtFrame, connected ? 1 : 0);
  return String(json);
}

// Sends a 302 redirect to the given URL, with headers that stop the OS
// caching the answer. Without those, some devices (Android especially)
// remember "no portal here" from a previous network and never bother
// asking again on reconnect.
static void sendRedirect(AsyncWebServerRequest* request, const String& location) {
  AsyncWebServerResponse* response = request->beginResponse(302);
  response->addHeader("Location", location);
  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  response->addHeader("Pragma", "no-cache");
  request->send(response);
}

// The normal case - send a probe request straight to our dashboard.
static void sendCaptivePortalRedirect(AsyncWebServerRequest* request) {
  sendRedirect(request, "/");
}

void TelemetryServer::begin(const char* mdnsName) {
  // mDNS is a nice-to-have - lets some devices use energymonitor.local
  // instead of typing in the IP. Not critical if it fails.
  if (MDNS.begin(mdnsName)) {
    Serial.printf("mDNS ready - http://%s.local\n", mdnsName);
  } else {
    Serial.println("mDNS failed to start - use the IP instead");
  }

  // Wire up WebSocket events to our handler function.
  ws_.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    handleWsEvent(server, client, type, arg, data, len);
  });
  server_.addHandler(&ws_);

  // Serve the dashboard page from flash so it loads straight off the ESP32.
  server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  // Plain HTTP endpoint for quick testing in a browser, e.g. http://192.168.4.1/status
  server_.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    bool connected = isConnectedNow();
    if (!last_) {
      // Haven't got a reading yet at all.
      char json[64];
      snprintf(json, sizeof(json), "{\"connected\":%d,\"error\":\"no reading yet\"}", connected ? 1 : 0);
      request->send(503, "application/json", json);
      return;
    }
    char json[192];
    snprintf(json, sizeof(json),
      "{\"v\":%.1f,\"i\":%.0f,\"p\":%.2f,\"ph\":%.1f,\"ageMs\":%lu,\"connected\":%d}",
      last_->voltage, last_->current, last_->power, last_->phase, millis() - last_->millisAtFrame, connected ? 1 : 0);
    request->send(200, "application/json", json);
  });

  // --- Captive portal setup, part 1: known probe URLs ---
  // Phones/laptops hit a few well-known URLs right after connecting to
  // check if there's real internet. Registering these by name (instead
  // of relying only on onNotFound below) lets us attach the no-cache
  // headers, which some OSes need before they'll actually show the
  // "Sign in to network" prompt.
  static const char* const kCaptivePortalPaths[] = {
    "/generate_204",               // Android
    "/gen_204",                    // Android (newer)
    "/ncsi.txt",                   // Windows
    "/redirect",                   // Windows - opened when the user clicks the "Action required" notification
    "/hotspot-detect.html",        // Apple (iOS + macOS)
    "/library/test/success.html",  // Apple (older)
    "/canonical.html",             // Firefox / Ubuntu NetworkManager
  };
  for (const char* path : kCaptivePortalPaths) {
    server_.on(path, HTTP_GET, sendCaptivePortalRedirect);
  }

  // Windows 11 quirk: redirecting /connecttest.txt straight to us like
  // every other probe sometimes makes NCSI decide there's no internet
  // at all and give up silently, instead of showing the "Action
  // required" notification. Pointing it at an external, unreachable
  // domain instead reliably triggers that notification - when the user
  // clicks it, Windows opens /redirect (handled above) for real.
  server_.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendRedirect(request, "http://logout.net");
  });

  // Browsers ask for this automatically the first time the dashboard
  // loads - a quick 404 stops it falling through to onNotFound and
  // getting treated as another captive-portal probe.
  server_.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(404);
  });

  // Catch-all for anything not listed above - same redirect treatment,
  // makes each OS's internet check fail in its own way and pop the
  // sign-in prompt which opens our dashboard.
  server_.onNotFound(sendCaptivePortalRedirect);

  // --- Captive portal setup, part 2: fake DNS ---
  // We've got no real DNS to hand out, so just answer every lookup on
  // the AP with our own IP. NoError (rather than a failure code some
  // devices treat as "give up checking") + TTL 0 (no caching) so a
  // reconnecting phone always re-checks instead of using a stale answer.
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.setTTL(0);
  dns_.start(53, "*", WiFi.softAPIP());

  server_.begin();
  Serial.println("WebSocket server up at /ws (dashboard + captive portal at /)");
}

// Pushes a reading out to every client currently connected.
void TelemetryServer::broadcast(const Reading& r) {
  ws_.textAll(toJson(r, isConnectedNow()));
}

// Sends the current status/last reading to everyone every
// HEARTBEAT_INTERVAL_MS, even with no new reading. Without this, once
// the ATmega goes quiet there's nothing to trigger a broadcast, and the
// dashboard would just sit on "Connected" forever with stale numbers.
void TelemetryServer::maybeSendHeartbeat() {
  unsigned long now = millis();
  if (now - lastHeartbeatMs_ < HEARTBEAT_INTERVAL_MS) return;   // not time yet
  lastHeartbeatMs_ = now;
  static const Reading kBlank;
  ws_.textAll(toJson(last_ ? *last_ : kBlank, isConnectedNow()));
}

// Handles WebSocket connect/disconnect/etc events.
void TelemetryServer::handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Send current status straight away so the page doesn't sit on
    // "Connecting..." until the next heartbeat/reading comes in.
    static const Reading kBlank;
    client->text(toJson(last_ ? *last_ : kBlank, isConnectedNow()));
  }
}