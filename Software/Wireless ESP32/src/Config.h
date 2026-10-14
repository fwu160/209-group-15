#pragma once
#include <IPAddress.h>

// All the settings in one place so there's no magic numbers in the other files
namespace Config {

  // ESP32 makes its own WiFi network, so it works the same wherever we demo it
  // No password so a phone can join straight away
  constexpr const char* AP_SSID = "Team15_EnergyMonitor";

  // Not 192.168.x.x on purpose, Samsung phones don't show the sign-in popup for that range
  const IPAddress AP_LOCAL_IP(4, 3, 2, 1);
  const IPAddress AP_GATEWAY_IP(4, 3, 2, 1);
  const IPAddress AP_SUBNET(255, 255, 255, 0);

  // Lets some devices use energymonitor.local instead of the IP
  constexpr const char* MDNS_NAME = "energymonitor";

  // UART link to the ATmega, baud has to match the ATmega
  constexpr unsigned long ATMEGA_BAUD = 9600;
  constexpr int ATMEGA_RX_PIN = 16;   // ESP RX <- ATmega TX (through the 5V -> 3.3V divider)
  constexpr int ATMEGA_TX_PIN = -1;   // Not used, we never send anything back (GPIO17 if we ever do)

  // No frame for this long = ATmega has dropped off (status dot and USB printing)
  // ATmega sends every 0.5 s, so 2.5 s is plenty of slack
  constexpr unsigned long ATMEGA_TIMEOUT_MS = 2500;

}