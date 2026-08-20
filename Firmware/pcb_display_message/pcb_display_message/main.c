/*
 * pcb_display_message.c
 *
 * Created: 20/08/2026 11:14:03
 * Author : Team 15
 */ 

#define F_CPU 2000000UL
#define BAUD 9600
#define UBRR ((F_CPU / (16UL * BAUD)) - 1)

#define RMSVoltage   14.5
#define PeakCurrent  125
#define Power        1.60

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "uart.h"

int main(void)
{
	uart_init(UBRR);
	
	while (1)
	{
		transmit_voltage_message(RMSVoltage);
		transmit_current_message(PeakCurrent);
		transmit_power_message(Power);
		
		// Print an extra newline to match the requested blank line separator
		uart_transmit('\r');
		uart_transmit('\n');
		
		_delay_ms(1000);
	}
}