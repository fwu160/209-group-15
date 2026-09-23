#include "UartBridge.h"

// Opens the serial port at the given baud/pins so we're ready to receive.
void UartBridge::begin(HardwareSerial& serial, unsigned long baud, int rxPin, int txPin) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
}

// Reads whatever's in the UART buffer, one character at a time, and
// builds up a line until we hit '\n'.
void UartBridge::poll() {
  if (!serial_) return;   // begin() hasn't been called, nothing to do

  while (serial_->available()) {
    char c = serial_->read();
    if (c == '\n') {
      // Got a full line - clean it up and hand it off.
      buffer_.trim();
      if (buffer_.length() > 0) handleLine(buffer_);
      buffer_ = "";   // reset for the next line
    } else if (c != '\r') {
      buffer_ += c;
      if (buffer_.length() > 96) buffer_ = "";  // line's gone way too long, bin it and start again
    }
  }
}

// Expects "voltage,current,power" as a CSV line. Update this if the
// ATmega ever changes what it sends.
void UartBridge::handleLine(const String& line) {
  // Find the two commas so we can slice the string into 3 fields.
  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);
  if (c1 < 0 || c2 < 0) return;  // malformed frame, just drop it

  last_.voltage = line.substring(0, c1).toFloat();
  last_.current = line.substring(c1 + 1, c2).toFloat();
  last_.power   = line.substring(c2 + 1).toFloat();
  last_.millisAtFrame = millis();   // stamp it so isConnected()/timeout logic works

  // Tell whoever's listening (main.cpp) that a new reading is ready.
  if (callback_) callback_(last_);
}