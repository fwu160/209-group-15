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
void transmit_voltage_message(float voltage);
void transmit_current_message(float current);
void transmit_power_message(float power);
void uart_transmit_string(char* message);
void transmit_uart_frame(float voltage, float current, float power);




#endif /* UART_H_ */