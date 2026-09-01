/*
 * ESP32 wireless bridge - ELECTENG 209 Smart Energy Challenge
 * Gets readings from the ATmega328P over UART, sends them out over
 * WiFi (to the dashboard website) and USB.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "UartBridge.h"
#include "TelemetryServer.h"

UartBridge uartBridge;
TelemetryServer telemetryServer;

// Prints a reading out the USB port in the required format
void printReadingOverUsb(const Reading& r) {
  Serial.printf("RMS Voltage is: %.1f\r\n", r.voltage);
  Serial.printf("Peak Current is: %.0f\r\n", r.current);
  Serial.printf("Power is: %.2f\r\n", r.power);
  Serial.println(); // Blank line before the next block
}

// Called whenever uartBridge parses a new frame from the ATmega.
void onNewReading(const Reading& r) {
  telemetryServer.broadcast(r);
  printReadingOverUsb(r);
}

// Starts the ESP32's own WiFi network.
void startAccessPoint() {
  WiFi.mode(WIFI_AP);   // ESP32 makes its own network
  WiFi.softAP(Config::AP_SSID, Config::AP_PASSWORD);

  Serial.println("Access point started.");
  Serial.print("  SSID: ");
  Serial.println(Config::AP_SSID);
  Serial.print("  Connect then go to http://");
  Serial.println(WiFi.softAPIP());   // Usually 192.168.4.1
}

void setup() {
  Serial.begin(9600);   // USB output at 9600 baud

  // Set up the UART link before starting the access point, so its ready to receive from the ATmega as soon as it starts sending.
  uartBridge.begin(Serial2, Config::ATMEGA_BAUD, Config::ATMEGA_RX_PIN, Config::ATMEGA_TX_PIN);
  uartBridge.onReading(onNewReading);

  startAccessPoint();

  telemetryServer.setLastReadingProvider(&uartBridge.lastReading());
  telemetryServer.begin(Config::MDNS_NAME);
}

void loop() {
  uartBridge.poll();      // Check for new UART data from the ATmega
  telemetryServer.loop(); // WebSocket housekeeping
}