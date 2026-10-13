/*
 * zerocross.c
 *
 * Created: 9/23/2026 8:34:29 PM
 *  Author: hiimf
 */ 


#include <avr/interrupt.h>
#include "zerocross.h"
#include "timer.h"

static volatile uint8_t zc_flag = 0;


// Rising edge, but INT0 stays off until zc_wait_for_edge() otherwise an early edge could start timer1 too soon
void zc_init(void){			
	DDRD &= ~ (1<<DDD2);			// Set PD2 as input, INT0 pin 	
	EICRA |= (1<<ISC01)|(1<<ISC00);	// Set interrupt to be rising edge ISC bits => 11
}

// Used by capture file to make sure both V and I bursts are taken from the relative same point on the V wave
uint8_t zc_wait_for_edge(void)
{
	uint16_t timeout = ZC_TIMEOUT_LOOPS;

	zc_flag = 0;             // Reset the "did an edge happen" flag
	EIFR  = (1 << INTF0);    // Clear any STALE pending edge from before we were ready
	EIMSK |= (1 << INT0);    // Turn on INT0 interrupt  
	
	// Loop that just checks if either zc flag is 0 or still time left before timeout, continue the block loop
	while (!zc_flag && --timeout) {
		;	
	}

	
	if (!zc_flag) {
		EIMSK &= ~(1 << INT0);   // Gave up - disarm so a late edge is ignored
		return 0;                // No signal
	}
	return 1;                    // Real edge was seen
}

ISR(INT0_vect)
{	
	
	timer1_start();		// Start timer1, will be used to set adc sampling frequency 
	
	// Ignore any further edges until zc_wait_for_edge() calls this again for the next burst.
	
	EIMSK &= (uint8_t)~(1 << INT0);
	
	zc_flag = 1;
}