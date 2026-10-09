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


static volatile uint8_t report_ticks = 0;	// how many 99.84 ms ticks since the last report
static volatile uint8_t report_flag  = 0;	// set to 1 by the ISR when a report is due

void timer2_init(void)
{
	TCCR2A = (1 << WGM21);							// CTC mode: count 0..OCR2A then start again (WGM2 bits = 010)
	OCR2A  = TIMER2_TOP;							// 194 -> 195 counts per cycle
	TIMSK2 = (1 << OCIE2A);							// interrupt on compare match A
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);	// clk/1024 (Timer2's prescaler table: 111 = 1024). This starts the timer.
}

// Runs every 99.84 ms, can interrupt an ADC burst but small enough so it should be fine 
ISR(TIMER2_COMPA_vect)
{
	report_ticks++;
	if (report_ticks >= TICKS_PER_REPORT) {
		report_ticks = 0;
		report_flag = 1;
	}
}

// Main calls this every loop. Returns 1 once per report period (TICKS_PER_REPORT ticks).
uint8_t timer2_report_due(void)
{
	if (report_flag) {
		report_flag = 0;	// Clear it so the next call returns 0 until the next period is up
		return 1;
	}
	return 0;
}