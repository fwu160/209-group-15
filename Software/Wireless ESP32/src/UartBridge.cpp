#include "UartBridge.h"

// Opens the serial port so we're ready to receive
void UartBridge::begin(HardwareSerial& serial, unsigned long baud, int rxPin, int txPin) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
}

// Builds up a line one character at a time until '\n'
void UartBridge::poll() {
  if (!serial_) return;   // Not started yet (no begin())

  while (serial_->available()) {
    char c = serial_->read();
    if (c == '\n') {
      // Full line, tidy it up and parse it
      buffer_.trim();
      if (buffer_.length() > 0) handleLine(buffer_);
      buffer_ = "";
    } else if (c != '\r') {
      buffer_ += c;
      if (buffer_.length() > 96) buffer_ = "";  // Way too long, must be junk so start again
    }
  }
}

// Expects "voltage,current,power,phase", change this if the ATmega frame changes
void UartBridge::handleLine(const String& line) {
  // Find the commas to split the 4 fields
  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);
  if (c1 < 0 || c2 < 0) return;  // Bad frame, drop it
  int c3 = line.indexOf(',', c2 + 1);   // Gives -1 if there's no phase field

  last_.voltage = line.substring(0, c1).toFloat();
  last_.current = line.substring(c1 + 1, c2).toFloat();
  if (c3 < 0) {
    // Old 3 field frame without phase, still accept it
    last_.power = line.substring(c2 + 1).toFloat();
    last_.phase = 0;
  } else {
    last_.power = line.substring(c2 + 1, c3).toFloat();
    last_.phase = line.substring(c3 + 1).toFloat();
  }
  last_.millisAtFrame = millis();   // Used for the connected timeout

  // Let main.cpp know there's a new reading
  if (callback_) callback_(last_);
}