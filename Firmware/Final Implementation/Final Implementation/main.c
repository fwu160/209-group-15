/*
 * Final Implementation.c
 *
 * Created: 9/7/2026 3:55:59 PM
 * Author : 
 */ 

#include <avr/io.h>



#define   RMSVoltage     14.5
#define   PeakCurrent    125
#define   Power          1.60

#include "common.h"
#include "uart.h"



int main(void)
{
	uart_init(UBRR); // Initiate uart 
    while (1) 
    {	

		transmit_uart_frame(RMSVoltage, PeakCurrent, Power);  // sends to the ESP32

		_delay_ms(1000);	// Wait a second before looping back to send messages
    }
}

