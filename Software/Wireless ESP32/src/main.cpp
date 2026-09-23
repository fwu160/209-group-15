/*
 * ESP32 wireless bridge - ELECTENG 209 Smart Energy Challenge
 * Gets readings from the ATmega328P over UART, sends them out over
 * WiFi (to the dashboard website) and USB.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>   // needed for the AMPDU RX workaround below
#include "Config.h"
#include "UartBridge.h"
#include "TelemetryServer.h"

UartBridge uartBridge;
TelemetryServer telemetryServer;

// Prints a reading out the USB serial port, formatted the way the spec wants.
void printReadingOverUsb(const Reading& r) {
  Serial.printf("RMS Voltage is: %.1f\r\n", r.voltage);
  Serial.printf("Peak Current is: %.0f\r\n", r.current);
  Serial.printf("Power is: %.2f\r\n", r.power);
  Serial.println(); // blank line before the next block
}

// Runs whenever uartBridge finishes parsing a new frame from the ATmega.
// Just fans it out to both outputs.
void onNewReading(const Reading& r) {
  telemetryServer.broadcast(r);
  printReadingOverUsb(r);
}

// True if the ATmega's sent a frame recently, i.e. it's actually alive
// and talking, not just idling. Drives the dashboard's status dot.
bool isAtmegaConnected() {
  return uartBridge.isConnected(Config::ATMEGA_TIMEOUT_MS);
}

// Starts the ESP32's own WiFi access point (open, no password) so a
// phone can join straight from the "Sign in to network" prompt.
void startAccessPoint() {
  WiFi.mode(WIFI_AP);   // ESP32 hosts its own network

  // Give the AP the public-looking IP from Config instead of the usual
  // 192.168.4.1 - see the comment on Config::AP_LOCAL_IP for why (fixes
  // the captive portal popup not showing up on Samsung phones). Has to
  // be set before softAP() actually starts broadcasting.
  WiFi.softAPConfig(Config::AP_LOCAL_IP, Config::AP_GATEWAY_IP, Config::AP_SUBNET);
  WiFi.softAP(Config::AP_SSID);    // no password arg = open network

  // Known ESP32 WiFi driver bug: AMPDU frame aggregation on the RX side
  // trips up some Android phones and causes them to silently drop or
  // never fully complete the connection to the AP. Restarting the WiFi
  // driver with it turned off fixes it - the AP config we just set
  // above stays intact through the restart. Costs us a bit of WiFi
  // throughput we don't need anyway.
  esp_wifi_stop();
  esp_wifi_deinit();
  wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
  wifiConfig.ampdu_rx_enable = false;
  esp_wifi_init(&wifiConfig);
  esp_wifi_start();
  delay(100);   // give the WiFi driver a moment to settle before we start using it

  Serial.println("Access point started.");
  Serial.print("  SSID: ");
  Serial.println(Config::AP_SSID);
  Serial.print("  Connect then go to http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(9600);   // USB output at 9600 baud

  // UART needs to be ready before the AP goes up, so we don't miss the
  // ATmega's first frames.
  uartBridge.begin(Serial2, Config::ATMEGA_BAUD, Config::ATMEGA_RX_PIN, Config::ATMEGA_TX_PIN);
  uartBridge.onReading(onNewReading);

  startAccessPoint();

  // Hook the server up to the UART bridge so it can answer /status and
  // report the real connection state.
  telemetryServer.setLastReadingProvider(&uartBridge.lastReading());
  telemetryServer.setConnectedProvider(isAtmegaConnected);
  telemetryServer.begin(Config::MDNS_NAME);
}

void loop() {
  uartBridge.poll();      // check for new UART data from the ATmega
  telemetryServer.loop(); // WebSocket/DNS housekeeping + heartbeat
}