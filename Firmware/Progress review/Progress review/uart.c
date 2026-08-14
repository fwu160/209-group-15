/*
 * uart.c
 *
 * Created: 8/14/2026 12:10:59 PM
 *  Author: 
 */ 
#include "uart.h"

// Configures the UART to 9600 baud (at F_CPU=2MHz), 8 data bits, 1 stop bit, no parity (8N1)
void uart_init(uint16_t ubrr) {
	UBRR0 = ubrr; 
	
	UCSR0A = 0x00;							// nothing needs to be set
	UCSR0B |= (1 << TXEN0);					// Enable Transmitter
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);	// Frame format: Async, No Parity, 1 Stop Bit, 8 Data Bits (technically 11 for 8bit is default, so don't need to set)
}

// Transmits a single byte via UART
void uart_transmit(uint8_t data) {
	
	while (!(UCSR0A & (1 << UDRE0))) {		// Wait until transmit buffer UDR0 is empty (UDRE0 bit becomes 1)
		;// Wait
	}
	UDR0 = data;	// Load byte into data register to transmit
}