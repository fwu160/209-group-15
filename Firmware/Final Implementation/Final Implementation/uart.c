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
	
	UCSR0A = 0x00;							// Nothing needs to be set
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