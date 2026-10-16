#pragma once
#include <Arduino.h>

// One reading from the ATmega
struct Reading {
  float voltage = 0;                 // Vrms in V
  float current = 0;                 // Peak current in mA
  float power = 0;                   // Real power in W
  float phase = 0;                   // Phase between V and I in degrees
  float energy = 0;                  // Energy since ATmega power-up in J
  unsigned long millisAtFrame = 0;   // Time the frame arrived, from millis()
};

// Function type that gets called with each new reading
using ReadingCallback = void (*)(const Reading&);

// Reads lines like "13.81,654,5.54,27.73,1234.5" from the ATmega and turns them into Readings
// Doesn't block, just call poll() every loop()
class UartBridge {
public:
  // Starts the serial port on the given pins
  void begin(HardwareSerial& serial, unsigned long baud, int rxPin, int txPin);

  // Reads any waiting bytes and handles full lines
  void poll();

  // Sets the function to call for each new reading
  void onReading(ReadingCallback cb) { callback_ = cb; }

  // Latest reading
  const Reading& lastReading() const { return last_; }

  // True if a frame came in within the last timeoutMs
  bool isConnected(unsigned long timeoutMs) const {
    return last_.millisAtFrame != 0 && (millis() - last_.millisAtFrame) < timeoutMs;
  }

private:
  void handleLine(const String& line);   // Splits one line into last_

  HardwareSerial* serial_ = nullptr;
  String buffer_;             // Characters so far, waiting for '\n'
  Reading last_;
  ReadingCallback callback_ = nullptr;
};