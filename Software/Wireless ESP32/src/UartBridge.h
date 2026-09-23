#pragma once
#include <Arduino.h>

// One parsed measurement from the ATmega328P.
struct Reading {
  float voltage = 0;
  float current = 0;
  float power = 0;
  unsigned long millisAtFrame = 0;   // millis() timestamp of when this reading came in
};

// Callback type - gets called every time we finish parsing a new reading.
using ReadingCallback = void (*)(const Reading&);

// Listens on a UART port for lines like "230.42,2.351,541.87" and turns
// them into Reading structs. Non-blocking, just call poll() every loop().
class UartBridge {
public:
  // Sets up the serial port with the given baud rate + pins.
  void begin(HardwareSerial& serial, unsigned long baud, int rxPin, int txPin);

  // Grabs whatever bytes are waiting and processes any full lines found.
  void poll();

  // Register a function to run whenever a new reading comes in.
  void onReading(ReadingCallback cb) { callback_ = cb; }

  // Most recent reading we've got.
  const Reading& lastReading() const { return last_; }

  // True if a frame has arrived within the last timeoutMs, i.e. the
  // ATmega is actually alive and sending, not just sitting idle.
  bool isConnected(unsigned long timeoutMs) const {
    return last_.millisAtFrame != 0 && (millis() - last_.millisAtFrame) < timeoutMs;
  }

private:
  void handleLine(const String& line);   // parses one CSV line into last_

  HardwareSerial* serial_ = nullptr;
  String buffer_;             // chars collected so far, waiting for a '\n'
  Reading last_;
  ReadingCallback callback_ = nullptr;
};