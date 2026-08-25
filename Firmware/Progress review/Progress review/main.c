/*
 * Progress review.c
 *
 * Created: 8/13/2026 5:00:17 PM
 * Author : 
 */ 

#define F_CPU 2000000UL
#define BAUD 9600
#define UBRR ((F_CPU / (16UL * BAUD)) - 1)

#define   RMSVoltage     14.5
#define   PeakCurrent    125
#define   Power          1.60

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "uart.h"



int main(void)
{
	uart_init(UBRR); // Initiate uart 
    while (1) 
    {	
		transmit_voltage_message(RMSVoltage);  // Send voltage message across uart
		transmit_current_message(PeakCurrent); // Send current message across uart
		transmit_power_message(Power);         // Send power message across uart
		
		uart_transmit('\n'); // Line feed
		uart_transmit('\r')  // Carriage return
		
		_delay_ms(1000);	// Wait a second before looping back to send messages
    }
}

