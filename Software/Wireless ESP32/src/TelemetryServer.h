#pragma once
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "UartBridge.h"

// WiFi server + WebSocket that pushes readings to the dashboard website.
// Also serves the dashboard page itself, a /status endpoint for testing,
// and a captive-portal DNS trick so phones prompt to open the dashboard
// automatically instead of quietly dropping the "no internet" WiFi network.
class TelemetryServer {
public:
  // Starts the web server, WebSocket, mDNS, and captive-portal DNS.
  void begin(const char* mdnsName);

  // Call every loop() - cleans up disconnected WebSocket clients and
  // answers pending captive-portal DNS lookups.
  void loop() {
    ws_.cleanupClients();
    dns_.processNextRequest();
  }

  // Sends a reading to every connected WebSocket client.
  void broadcast(const Reading& r);

  // Lets /status and new clients answer with the latest reading right away.
  void setLastReadingProvider(const Reading* r) { last_ = r; }

private:
  AsyncWebServer server_{80};
  AsyncWebSocket ws_{"/ws"};
  DNSServer dns_;
  const Reading* last_ = nullptr;

  void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len);

  static String toJson(const Reading& r);
};