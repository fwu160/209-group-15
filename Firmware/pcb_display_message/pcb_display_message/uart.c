#include "uart.h"

// Configures the UART to 9600 baud (at F_CPU=2MHz), 8N1
void uart_init(uint16_t ubrr) {
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	
	UCSR0A = 0x00;
	UCSR0B |= (1 << TXEN0);                  // Enable Transmitter
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // Frame format: 8 Data Bits, 1 Stop Bit
}

// Transmits a single byte via UART
void uart_transmit(uint8_t data) {
	while (!(UCSR0A & (1 << UDRE0))) {
		; // Wait until transmit buffer UDR0 is empty
	}
	UDR0 = data;
}

// Transmits a null-terminated string
void uart_transmit_string(const char* message) {
	while (*message) {
		uart_transmit((uint8_t)*message++);
	}
}

// Transmits RMS Voltage: XX.X
void transmit_voltage_message(float voltage) {
	char buffer[40];
	int int_part = (int)voltage;
	int dec_part = (int)((voltage - int_part) * 10 + 0.5); // 1 decimal place with rounding
	
	sprintf(buffer, "RMS Voltage is: %d.%d\r\n", int_part, dec_part);
	uart_transmit_string(buffer);
}

// Transmits Peak Current: XXX
void transmit_current_message(uint16_t current) {
	char buffer[40];
	sprintf(buffer, "Peak Current is: %u\r\n", current);
	uart_transmit_string(buffer);
}

// Transmits Power: X.XX
void transmit_power_message(float power) {
	char buffer[40];
	int int_part = (int)power;
	int dec_part = (int)((power - int_part) * 100 + 0.5); // 2 decimal places with rounding
	
	// Handles leading zeros in fractional part (e.g., 1.05 -> 1.05 instead of 1.5)
	sprintf(buffer, "Power is: %d.%02d\r\n", int_part, dec_part);
	uart_transmit_string(buffer);
}