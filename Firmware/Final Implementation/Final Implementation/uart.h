/*
 * uart.h
 *
 * Created: 8/14/2026 12:11:24 PM
 *  Author:
 */ 


#ifndef UART_H_
#define UART_H_

#include "common.h"

// Function prototypes
void uart_init(uint16_t ubrr);
void uart_transmit(uint8_t data);
void uart_transmit_string(char* message);


#endif /* UART_H_ */