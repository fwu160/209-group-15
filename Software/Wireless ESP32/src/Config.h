#pragma once

// ESP32's own WiFi network. Creates its own AP instead of joining
// the school/home WiFi so it works the same wherever we are.
namespace Config {

  constexpr const char* AP_SSID     = "EnergyMonitor";
  constexpr const char* AP_PASSWORD = "energy123";   // Needs 8+ chars for WPA2

  // Backup way to reach it: http://energymonitor.local
  // Doesn't always work depending on OS, use the IP if it fails.
  constexpr const char* MDNS_NAME = "energymonitor";

  // UART link to the ATmega328P. Baud rate must match its USART setting.
  constexpr unsigned long ATMEGA_BAUD = 9600;
  constexpr int ATMEGA_RX_PIN = 16;   // ESP RX2 <- ATmega TXD (through voltage divider)
  constexpr int ATMEGA_TX_PIN = 17;   // NOT USED

}