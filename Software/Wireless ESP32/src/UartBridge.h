#pragma once
#include <Arduino.h>

// One parsed measurement from the ATmega328P.
struct Reading {
  float voltage = 0;
  float current = 0;
  float power = 0;
  unsigned long millisAtFrame = 0;   // When this reading came in
};

// Function called whenever a new reading is parsed.
using ReadingCallback = void (*)(const Reading&);

// Listens on a UART port for lines like "230.42,2.351,541.87" and
// parses them into Reading structs. Non-blocking - call poll() every loop().
class UartBridge {
public:
  // Sets up the given serial port at the given baud rate/pins.
  void begin(HardwareSerial& serial, unsigned long baud, int rxPin, int txPin);

  // Reads whatever bytes are available and processes any complete lines.
  void poll();

  // Registers a function to call whenever a new reading arrives.
  void onReading(ReadingCallback cb) { callback_ = cb; }

  // Returns the most recent reading received.
  const Reading& lastReading() const { return last_; }

private:
  void handleLine(const String& line);

  HardwareSerial* serial_ = nullptr;
  String buffer_;             // Characters collected so far, waiting for '\n'
  Reading last_;
  ReadingCallback callback_ = nullptr;
};