/*
 * ESP32 wireless bridge - ELECTENG 209 Smart Energy Monitor
 * Gets readings from the ATmega over UART and sends them to the dashboard (WiFi) and USB
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>   // For the AMPDU fix in startAccessPoint()
#include "Config.h"
#include "UartBridge.h"
#include "TelemetryServer.h"

UartBridge uartBridge;
TelemetryServer telemetryServer;

// Prints one reading over USB in the format the spec wants
void printReadingOverUsb(const Reading& r) {
  Serial.printf("RMS Voltage is: %.1f\r\n", r.voltage);
  Serial.printf("Peak Current is: %.0f\r\n", r.current);
  Serial.printf("Power is: %.2f\r\n", r.power);
  Serial.println(); // Blank line between blocks
}

// Runs for each new ATmega frame (every 0.5 s), sends it to the dashboard
// USB printing is done once a second in loop() instead
void onNewReading(const Reading& r) {
  telemetryServer.broadcast(r);
}

// True if the ATmega has sent a frame recently
bool isAtmegaConnected() {
  return uartBridge.isConnected(Config::ATMEGA_TIMEOUT_MS);
}

// Starts the ESP32's own open WiFi network
void startAccessPoint() {
  WiFi.mode(WIFI_AP);

  // IP has to be set before softAP() starts (see Config.h for why it's 4.3.2.1)
  WiFi.softAPConfig(Config::AP_LOCAL_IP, Config::AP_GATEWAY_IP, Config::AP_SUBNET);
  WiFi.softAP(Config::AP_SSID);    // No password = open network

  // ESP32 WiFi bug: AMPDU RX makes some Android phones fail to connect
  // Restart the driver with it off, the AP settings above are kept
  esp_wifi_stop();
  esp_wifi_deinit();
  wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
  wifiConfig.ampdu_rx_enable = false;
  esp_wifi_init(&wifiConfig);
  esp_wifi_start();
  delay(100);   // Let the driver settle

  Serial.println("Access point started.");
  Serial.print("  SSID: ");
  Serial.println(Config::AP_SSID);
  Serial.print("  Connect then go to http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(9600);   // USB output at 9600 baud

  // Start UART before WiFi so we don't miss the first frames
  uartBridge.begin(Serial2, Config::ATMEGA_BAUD, Config::ATMEGA_RX_PIN, Config::ATMEGA_TX_PIN);
  uartBridge.onReading(onNewReading);

  startAccessPoint();

  // Give the server the latest reading and the link status
  telemetryServer.setLastReadingProvider(&uartBridge.lastReading());
  telemetryServer.setConnectedProvider(isAtmegaConnected);
  telemetryServer.begin(Config::MDNS_NAME);
}

void loop() {
  uartBridge.poll();      // Check for new UART data
  telemetryServer.loop(); // WebSocket/DNS housekeeping and heartbeat

  // Print the latest reading over USB once a second
  // Only while the ATmega is sending, stops 2.5 s after the frames stop
  static unsigned long lastUsbPrintMs = 0;   // Static so it keeps its value between loop() runs
  if (millis() - lastUsbPrintMs >= 1000) {
    lastUsbPrintMs = millis();
    if (isAtmegaConnected()) {
      printReadingOverUsb(uartBridge.lastReading());
    }
  }
}