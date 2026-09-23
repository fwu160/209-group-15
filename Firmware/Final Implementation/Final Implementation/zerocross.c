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


// one-time setup: configure INT0 for a rising edge
void zc_init(void){			
	DDRD |= (1<<2);	// set PD2 as input as this is the INT0 pin 		
	EIMSK |= (1<<INT0);			// enable INT0 external interrupt 
	EICRA |= (1<<ISC01)|(1<<ISC00); // set interrupt to be rising edge ISC bits => 11
}

// used by capture file to make sure both V and I bursts are taken from the relative same point on the V wave
uint8_t zc_wait_for_edge(void)
{
	uint16_t timeout = ZC_TIMEOUT_LOOPS;

	zc_flag = 0;             // reset the "did an edge happen" flag
	EIFR  = (1 << INTF0);    // clear any STALE pending edge from before we were ready
	EIMSK |= (1 << INT0);    // turn on INT0 interput  
	
	// loop that just checks if either zc flag is 0 or still time left before timeout, continue the block loop
	while (!zc_flag && --timeout) {
		// just sit here
	}

	
	if (!zc_flag) {
		EIMSK &= ~(1 << INT0);   // gave up - disarm so a late edge is ignored
		return 0;                // failure: no signal
	}
	return 1;                    // success: a real edge was seen
}

ISR(INT0_vect)
{	
	// start timer1, will be used to set adc sampling frequency 
	timer1_start();
	
	// gnore any further edges until zc_wait_for_edge() calls this again for the next burst.
	
	EIMSK &= (uint8_t)~(1 << INT0);
	
	zc_flag = 1;
}