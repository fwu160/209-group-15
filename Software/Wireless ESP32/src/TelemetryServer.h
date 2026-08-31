#pragma once
#include <ESPAsyncWebServer.h>
#include "UartBridge.h"

// WiFi server + WebSocket that pushes readings to the dashboard website.
// Also has a /status endpoint for testing without the website.
class TelemetryServer {
public:
  // Starts the web server, WebSocket, and mDNS.
  void begin(const char* mdnsName);

  // Call every loop() - cleans up disconnected clients.
  void loop() { ws_.cleanupClients(); }

  // Sends a reading to every connected WebSocket client.
  void broadcast(const Reading& r);

  // Lets /status and new clients answer with the latest reading right away.
  void setLastReadingProvider(const Reading* r) { last_ = r; }

private:
  AsyncWebServer server_{80};
  AsyncWebSocket ws_{"/ws"};
  const Reading* last_ = nullptr;

  void handleWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len);

  static String toJson(const Reading& r);
};