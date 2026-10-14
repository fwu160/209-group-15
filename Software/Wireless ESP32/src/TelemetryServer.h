#pragma once
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "UartBridge.h"

// Web server for the dashboard, pushes live readings to it over a WebSocket
// Also has /status for testing and a captive portal so phones open the dashboard on joining
class TelemetryServer {
public:
  // Function type that says if the ATmega link is up (sent as "c" in every message)
  using ConnectedProvider = bool (*)();

  // Starts the web server, WebSocket, mDNS and captive portal DNS
  void begin(const char* mdnsName);

  // Call every loop(), cleans up clients, answers DNS and sends the heartbeat
  void loop() {
    ws_.cleanupClients();
    dns_.processNextRequest();
    maybeSendHeartbeat();
  }

  // Sends a reading to every connected browser
  void broadcast(const Reading& r);

  // So /status and new browsers can get the latest reading straight away
  void setLastReadingProvider(const Reading* r) { last_ = r; }

  // So messages report the real ATmega link status
  void setConnectedProvider(ConnectedProvider p) { connectedProvider_ = p; }

private:
  AsyncWebServer server_{80};
  AsyncWebSocket ws_{"/ws"};
  DNSServer dns_;
  const Reading* last_ = nullptr;
  ConnectedProvider connectedProvider_ = nullptr;
  unsigned long lastHeartbeatMs_ = 0;
  static constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;   // Status update even when nothing new comes in

  void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len);
  void maybeSendHeartbeat();
  bool isConnectedNow() const { return connectedProvider_ ? connectedProvider_() : (last_ != nullptr); }

  static String toJson(const Reading& r, bool connected);   // Builds the JSON message
};