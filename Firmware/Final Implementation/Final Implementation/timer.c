/*
 * timer.c
 *
 * Created: 9/23/2026 8:33:53 PM
 *  Author: hiimf
 */ 

#include "timer.h"
#include <avr/interrupt.h>

// CTC mode, TOP = TIMER1_TOP, left stopped
void timer1_init(void){
	TCCR1B |= (1<<WGM12);	// CTC mode set WGM bits 0100
	OCR1A = TIMER1_TOP;		// sets the period of the timer, TIMER1_TOP defined in common.h file 
	OCR1B = TIMER1_TOP;		// Needed since ADC only has a Timer/Counter1 Compare Match B trigger source 
	TIMSK1 = 0b00000000;	// No interrupts set 
	
}


// function to start timer1
void timer1_start(void){
	TCCR1B |= (1<<CS10);	// Set CS1 bits to 001, prescaler of 1 set (no prescaling)
}	


void timer1_stop(void){
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));	// set CS1 bits to 000, no clk source so timer1 is off
}

void timer1_reset(void)
{	
	
	TCNT1 = 0;					// Reset timer1 counter to 0
	TIFR1 = (1 << OCF1B);		// Clear compare b flag 
}
