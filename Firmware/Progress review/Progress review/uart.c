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

// Function turns a char array into their respective ascii values and transmits the array
void uart_transmit_string(char* message){
	// Char array to store ascii values 
	char string_buffer[30]; 
	// Stores string into a char array 
	sprintf(string_buffer,message); 
	
	// Loops through each value in array and transmits it
	for (int i = 0; i<strlen(string_buffer); i++){
		uart_transmit(string_buffer[i]);
	}
	
}

void transmit_voltage_message(float voltage){
	// Initial message to send before sending voltage value
	char* message = "RMS Voltage is: ";
	
	uart_transmit_string(message); 
	
	// Offset of 48 to get the correct ascii code value for the digits
	int8_t offset = 48; 
	// Splitting and transmitting voltage to 1 decimal place
	uart_transmit(((int)voltage / 10) + offset);	// 10's digit
	uart_transmit(((int)voltage % 10) + offset);	// 1's digit
	uart_transmit('.');
	uart_transmit((voltage * 10 - (int)voltage * 10) + offset);	// 0.1ths digit
	
	uart_transmit(13); // New line /n ascii code 


}