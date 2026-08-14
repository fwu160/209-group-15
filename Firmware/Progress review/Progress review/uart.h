/*
 * uart.h
 *
 * Created: 8/14/2026 12:11:24 PM
 *  Author:
 */ 


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

#define BAUD 9600
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1)

// Function prototypes
void uart_init(uint16_t ubrr);
void uart_transmit(uint8_t data);

#endif /* UART_H_ */