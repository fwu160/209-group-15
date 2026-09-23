#pragma once
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "UartBridge.h"

// Runs the web server + WebSocket that pushes live readings to the
// dashboard page. Also serves the dashboard HTML itself, a /status
// endpoint for quick testing, and a captive-portal DNS trick so phones
// auto-prompt to open the dashboard when they join the AP.
class TelemetryServer {
public:
  // Function pointer type for checking if the ATmega link is currently up
  // (see UartBridge::isConnected()). Gets sent to clients as the "c" field
  // on every message so the status dot reflects the real UART link, not
  // just whether a browser's WebSocket happens to be open.
  using ConnectedProvider = bool (*)();

  // Kicks off the web server, WebSocket, mDNS, and captive-portal DNS.
  void begin(const char* mdnsName);

  // Call this every loop() - handles WebSocket client cleanup, answers
  // pending DNS lookups, and fires off the periodic status heartbeat.
  void loop() {
    ws_.cleanupClients();
    dns_.processNextRequest();
    maybeSendHeartbeat();
  }

  // Pushes a reading out to every connected client.
  void broadcast(const Reading& r);

  // So /status and newly-connected clients can answer with the latest reading straight away.
  void setLastReadingProvider(const Reading* r) { last_ = r; }

  // So broadcasts/heartbeats/new clients can report the real ATmega link
  // status instead of just assuming everything's connected.
  void setConnectedProvider(ConnectedProvider p) { connectedProvider_ = p; }

private:
  AsyncWebServer server_{80};
  AsyncWebSocket ws_{"/ws"};
  DNSServer dns_;
  const Reading* last_ = nullptr;
  ConnectedProvider connectedProvider_ = nullptr;
  unsigned long lastHeartbeatMs_ = 0;
  static constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;   // how often we ping clients with status even if nothing new happened

  void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len);
  void maybeSendHeartbeat();
  bool isConnectedNow() const { return connectedProvider_ ? connectedProvider_() : (last_ != nullptr); }

  static String toJson(const Reading& r, bool connected);   // builds the JSON blob we send over the socket
};