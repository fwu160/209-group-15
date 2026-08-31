#include "UartBridge.h"

// Configures the serial port for the ATmega link.
void UartBridge::begin(HardwareSerial& serial, unsigned long baud, int rxPin, int txPin) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
}

// Reads any bytes waiting on the port, one line at a time.
void UartBridge::poll() {
  if (!serial_) return;

  while (serial_->available()) {
    char c = serial_->read();
    if (c == '\n') {
      // End of a line - process it, then start collecting the next one.
      buffer_.trim();
      if (buffer_.length() > 0) handleLine(buffer_);
      buffer_ = "";
    } else if (c != '\r') {
      buffer_ += c;
      if (buffer_.length() > 96) buffer_ = "";  // Something went wrong, reset
    }
  }
}

// Parses "voltage,current,power" - change this if the ATmega's format changes.
void UartBridge::handleLine(const String& line) {
  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);
  if (c1 < 0 || c2 < 0) return;  // Bad frame, drop it

  last_.voltage = line.substring(0, c1).toFloat();
  last_.current = line.substring(c1 + 1, c2).toFloat();
  last_.power   = line.substring(c2 + 1).toFloat();
  last_.millisAtFrame = millis();

  // Let main.cpp know a new reading is ready.
  if (callback_) callback_(last_);
}