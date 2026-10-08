#pragma once
#include <IPAddress.h>

// All the settings/constants for the project live here so we don't have
// magic numbers scattered everywhere.
namespace Config {

  // ESP32 makes its own WiFi network (access point) instead of joining
  // the uni/home WiFi, so it works the same no matter where we demo it.
  constexpr const char* AP_SSID = "EnergyMonitor";

  // No password = open network, so a phone can just join without typing
  // anything. If we want one later: WiFi.softAP(Config::AP_SSID, "pw") in main.cpp
  // (see startAccessPoint()).

  // IP/gateway we give the AP (set in startAccessPoint() in main.cpp).
  // Deliberately NOT a normal 192.168.x.x address - Samsung phones
  // check the gateway IP as part of deciding whether to show the
  // "Sign in to network" popup, and if it looks like a typical home
  // router range they assume it's a known network with a temporary
  // internet outage and never bother asking. Giving it a public-looking
  // address (this one's unused/reserved, so it won't clash with
  // anything) makes Samsung treat it like a proper hotspot and pop the
  // captive portal prompt like every other phone already does.
  const IPAddress AP_LOCAL_IP(4, 3, 2, 1);
  const IPAddress AP_GATEWAY_IP(4, 3, 2, 1);
  const IPAddress AP_SUBNET(255, 255, 255, 0);

  // Backup way to reach the dashboard if the IP isn't handy.
  // Doesn't work on every OS, so the IP is the reliable fallback.
  constexpr const char* MDNS_NAME = "energymonitor";

  // --- UART link to the ATmega328P ---
  // Baud has to match whatever the ATmega's USART is set to.
  constexpr unsigned long ATMEGA_BAUD = 9600;
  constexpr int ATMEGA_RX_PIN = 16;   // ESP RX2 <- ATmega TXD (through voltage divider, 5V->3.3V)
  constexpr int ATMEGA_TX_PIN = -1; // 17;   // ESP TX2 -> ATmega RXD (we don't actually send anything back)

  // How long we wait without a new frame before deciding the ATmega has
  // dropped off. This is what feeds UartBridge::isConnected() and drives
  // the Connected/Disconnected dot on the dashboard. ATmega sends ~once
  // a second, so 2.5s gives a bit of slack without being slow to notice.
  constexpr unsigned long ATMEGA_TIMEOUT_MS = 2500;

}