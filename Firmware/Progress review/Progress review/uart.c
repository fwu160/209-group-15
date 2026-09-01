/*
 * uart.c
 *
 * Created: 8/14/2026 12:10:59 PM
 *  Author: 
 */ 


#include "uart.h"

// Configures the UART baud rate (via ubrr), 8 data bits, 1 stop bit, no parity (8N1)
void uart_init(uint16_t ubrr) {
	UBRR0 = ubrr; 
	
	UCSR0A = 0x00;							// nothing needs to be set
	UCSR0B |= (1 << TXEN0);					// Enable Transmitter
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);	// Frame format: Async, No Parity, 1 Stop Bit, 8 Data Bits (UCSZ02 in UCSR0B defaults to 0)
}

// Transmits a single byte via UART
void uart_transmit(uint8_t data) {
	
	while (!(UCSR0A & (1 << UDRE0))) {		// Wait until transmit buffer UDR0 is empty (UDRE0 bit becomes 1)
		;// Wait
	}
	UDR0 = data;	// Load byte into data register to transmit
}

// Function iterates through a string array using pointer and sends it via uart 
void uart_transmit_string(char* message){
	// Loops through string array until null terminator is reached. 
	for (int i = 0; message[i] != '\0'; i++) {
		uart_transmit(message[i]);
	}
	
}

void transmit_voltage_message(float voltage){
	// Initial message to send before sending voltage value
	char* message = "RMS Voltage is: ";
	
	uart_transmit_string(message); 
	
	// Offset of 48 to get the correct ascii code value for the digits
	int8_t offset = 48; 
	// Splitting and transmitting voltage to 1 decimal place
	uart_transmit((((int)voltage / 10) % 10) + offset);    // 10's digit
	uart_transmit((((int)voltage) % 10) + offset);         // 1's digit
	uart_transmit('.');
	uart_transmit((((int)(voltage * 10)) % 10) + offset);  // 0.1ths digit

	uart_transmit('\r'); // Carriage return
	uart_transmit('\n'); // Line feed
	}

void transmit_current_message(float current){
	// Initial message to send before sending current value
	char* message = "Peak Current is: ";
	
	uart_transmit_string(message); 
	
	// Offset of 48 to get the correct ascii code value for the digits
	int8_t offset = 48; 
	// Splitting and transmitting current 
	uart_transmit((((int)current / 100) % 10) + offset); // 100's digit
	uart_transmit((((int)current / 10) % 10) + offset);  // 10's digit 
	uart_transmit((((int)current) % 10) + offset);       // 1's digit

	uart_transmit('\r');
	uart_transmit('\n');
}

void transmit_power_message(float power){
	// Initial message to send before sending power value
	char* message = "Power is: ";
	
	uart_transmit_string(message); 
	
	// Offset of 48 to get the correct ascii code value for the digits
	int8_t offset = 48; 
	// Splitting and transmitting power to 2 decimal place
	uart_transmit((((int)power) % 10) + offset);         // 1's digit
	uart_transmit('.');
	uart_transmit((((int)(power * 10)) % 10) + offset);  // 0.1ths digit
	uart_transmit((((int)(power * 100)) % 10) + offset); // 0.01ths digit (fixed formula)

	uart_transmit('\r');
	uart_transmit('\n');

}

// Sends "voltage,current,power\n" - the compact format the ESP32 bridge parses.
// Different from the transmit_*_message functions above, which build the
// human-readable text meant for a terminal, not for the ESP link.
void transmit_uart_frame(float voltage, float current, float power) {
	int8_t offset = 48;

	// Voltage: XX.X
	uart_transmit((((int)voltage / 10) % 10) + offset);
	uart_transmit((((int)voltage) % 10) + offset);
	uart_transmit('.');
	uart_transmit((((int)(voltage * 10)) % 10) + offset);

	uart_transmit(',');

	// Current: XXX (already in mA, integer, no decimal point)
	uart_transmit((((int)current / 100) % 10) + offset);
	uart_transmit((((int)current / 10) % 10) + offset);
	uart_transmit((((int)current) % 10) + offset);

	uart_transmit(',');

	// Power: X.XX
	uart_transmit((((int)power) % 10) + offset);
	uart_transmit('.');
	uart_transmit((((int)(power * 10)) % 10) + offset);
	uart_transmit((((int)(power * 100)) % 10) + offset);

	uart_transmit('\n');
}