#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Function prototypes
void uart_init(uint16_t ubrr);
void uart_transmit(uint8_t data);
void uart_transmit_string(const char* message);

void transmit_voltage_message(float voltage);
void transmit_current_message(uint16_t current);
void transmit_power_message(float power);

#endif /* UART_H_ */