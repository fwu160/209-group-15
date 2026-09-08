#include "TelemetryServer.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include "DashboardPage.h"

// Builds the JSON string sent to the website, e.g. {"v":14.5,"i":300,"p":6.43,"t":123456}
String TelemetryServer::toJson(const Reading& r) {
  char json[128];
  snprintf(json, sizeof(json),
    "{\"v\":%.1f,\"i\":%.0f,\"p\":%.2f,\"t\":%lu}",
    r.voltage, r.current, r.power, r.millisAtFrame);
  return String(json);
}

void TelemetryServer::begin(const char* mdnsName) {
  // Optional - lets some devices reach us at energymonitor.local instead of the IP.
  if (MDNS.begin(mdnsName)) {
    Serial.printf("mDNS ready - http://%s.local\n", mdnsName);
  } else {
    Serial.println("mDNS failed to start - use the IP instead");
  }

  // Forward WebSocket events to our handler.
  ws_.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    handleWsEvent(server, client, type, arg, data, len);
  });
  server_.addHandler(&ws_);

  // Serve the dashboard page itself, so it loads straight from the ESP32
  // instead of needing basic.html copied onto whatever device you're using.
  server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  // Plain HTTP endpoint for testing with just a browser, e.g. http://192.168.4.1/status
  server_.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!last_) {
      request->send(503, "application/json", "{\"error\":\"no reading yet\"}");
      return;
    }
    char json[160];
    snprintf(json, sizeof(json),
      "{\"v\":%.1f,\"i\":%.0f,\"p\":%.2f,\"ageMs\":%lu}",
      last_->voltage, last_->current, last_->power, millis() - last_->millisAtFrame);
    request->send(200, "application/json", json);
  });

  // Captive portal, part 1: phones and laptops probe a handful of
  // well-known URLs right after connecting to check for real internet
  // access (e.g. /generate_204 on Android, /hotspot-detect.html on iOS,
  // /ncsi.txt on Windows). We don't register any of those specifically -
  // redirecting everything we don't otherwise recognise to "/" makes each
  // OS's probe fail in its own way, which is exactly what makes it show a
  // "Sign in to network" prompt that opens our dashboard automatically.
  server_.onNotFound([](AsyncWebServerRequest* request) {
    request->redirect("/");
  });

  // Captive portal, part 2: the redirect above only works if the probe
  // request reaches us in the first place. We have no real DNS to give
  // out, so instead we answer *every* DNS lookup from anyone on the AP
  // with our own IP - that's what actually triggers the "Sign in to
  // network" prompt instead of the OS just timing out silently.
  dns_.start(53, "*", WiFi.softAPIP());

  server_.begin();
  Serial.println("WebSocket server up at /ws (dashboard + captive portal at /)");
}

// Sends a reading to every connected client at once.
void TelemetryServer::broadcast(const Reading& r) {
  ws_.textAll(toJson(r));
}

// Handles WebSocket connect/disconnect/etc. events.
void TelemetryServer::handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT && last_) {
    client->text(toJson(*last_));  // Send the latest reading so the page isn't blank on load
  }
}