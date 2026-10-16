#include "TelemetryServer.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include "Dashboardpage.h"

// Turns a Reading into the JSON the dashboard reads
// Example: {"v":13.81,"i":654,"p":5.54,"ph":27.73,"e":1234.5,"t":123456,"c":1}
// The "t" field is when the frame arrived, "c" is 1 if the ATmega is sending
String TelemetryServer::toJson(const Reading& r, bool connected) {
  char json[144];
  snprintf(json, sizeof(json),
    "{\"v\":%.2f,\"i\":%.0f,\"p\":%.2f,\"ph\":%.2f,\"e\":%.1f,\"t\":%lu,\"c\":%d}",
    r.voltage, r.current, r.power, r.phase, r.energy, r.millisAtFrame, connected ? 1 : 0);
  return String(json);
}

// Redirect with no-cache headers, otherwise some phones remember "no portal here" and never ask again
static void sendRedirect(AsyncWebServerRequest* request, const String& location) {
  AsyncWebServerResponse* response = request->beginResponse(302);
  response->addHeader("Location", location);
  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  response->addHeader("Pragma", "no-cache");
  request->send(response);
}

// Sends a phone's internet check to the dashboard
static void sendCaptivePortalRedirect(AsyncWebServerRequest* request) {
  sendRedirect(request, "/");
}

void TelemetryServer::begin(const char* mdnsName) {
  // Optional, lets some devices use energymonitor.local
  if (MDNS.begin(mdnsName)) {
    Serial.printf("mDNS ready - http://%s.local\n", mdnsName);
  } else {
    Serial.println("mDNS failed to start - use the IP instead");
  }

  // Hook up WebSocket events
  ws_.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    handleWsEvent(server, client, type, arg, data, len);
  });
  server_.addHandler(&ws_);

  // Dashboard page, stored in flash
  server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  // Quick test in a browser: http://4.3.2.1/status
  server_.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    bool connected = isConnectedNow();
    if (!last_) {
      // No reading yet
      char json[64];
      snprintf(json, sizeof(json), "{\"connected\":%d,\"error\":\"no reading yet\"}", connected ? 1 : 0);
      request->send(503, "application/json", json);
      return;
    }
    char json[192];
    snprintf(json, sizeof(json),
      "{\"v\":%.2f,\"i\":%.0f,\"p\":%.2f,\"ph\":%.2f,\"e\":%.1f,\"ageMs\":%lu,\"connected\":%d}",
      last_->voltage, last_->current, last_->power, last_->phase, last_->energy, millis() - last_->millisAtFrame, connected ? 1 : 0);
    request->send(200, "application/json", json);
  });

  // Captive portal part 1: URLs phones check for internet right after joining
  // Listed by name so they get the no-cache headers
  static const char* const kCaptivePortalPaths[] = {
    "/generate_204",               // Android
    "/gen_204",                    // Android (newer)
    "/ncsi.txt",                   // Windows
    "/redirect",                   // Windows, opened when you click "Action required"
    "/hotspot-detect.html",        // Apple
    "/library/test/success.html",  // Apple (older)
    "/canonical.html",             // Firefox / Ubuntu
  };
  for (const char* path : kCaptivePortalPaths) {
    server_.on(path, HTTP_GET, sendCaptivePortalRedirect);
  }

  // Windows 11 needs an outside address here or it gives up silently
  // This makes it show "Action required", which then opens /redirect
  server_.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendRedirect(request, "http://logout.net");
  });

  // Browsers ask for this, send 404 so it isn't treated as a portal check
  server_.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(404);
  });

  // Anything else goes to the dashboard too
  server_.onNotFound(sendCaptivePortalRedirect);

  // Captive portal part 2: answer every DNS lookup with our own IP
  // NoError and TTL 0 so phones don't give up or cache the answer
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.setTTL(0);
  dns_.start(53, "*", WiFi.softAPIP());

  server_.begin();
  Serial.println("WebSocket server up at /ws (dashboard + captive portal at /)");
}

// Sends a reading to every connected browser
void TelemetryServer::broadcast(const Reading& r) {
  ws_.textAll(toJson(r, isConnectedNow()));
}

// Sends the status every second even with no new reading
// Without this the dot would stay green after the ATmega stops
void TelemetryServer::maybeSendHeartbeat() {
  unsigned long now = millis();
  if (now - lastHeartbeatMs_ < HEARTBEAT_INTERVAL_MS) return;   // Not time yet
  lastHeartbeatMs_ = now;
  static const Reading kBlank;
  ws_.textAll(toJson(last_ ? *last_ : kBlank, isConnectedNow()));
}

// WebSocket events
void TelemetryServer::handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // New browser gets the current status straight away
    static const Reading kBlank;
    client->text(toJson(last_ ? *last_ : kBlank, isConnectedNow()));
  }
}