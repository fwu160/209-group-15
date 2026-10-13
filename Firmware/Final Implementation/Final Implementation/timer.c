/*
 * timer.c
 *
 * Created: 9/23/2026 8:33:53 PM
 *  Author: hiimf
 */ 

#include "timer.h"
#include <avr/interrupt.h>

static volatile uint8_t report_ticks = 0;	// how many 99.84 ms ticks since the last report
static volatile uint8_t report_flag  = 0;	// set to 1 by the ISR when a report is due


// Fast PWM 64 prescaler 
// OC0B is connected to display OE pin, set high at count 0, cleared low at OCR0B
void timer0_init(void){

	OCR0A  = TIMER0_TOP;		
	TCCR0A = (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << WGM02) | (1 << CS01) | (1 << CS00);
	TIMSK0 = (1 << TOIE0); 
}

// CTC mode, TOP = TIMER1_TOP, left stopped
void timer1_init(void){
	TCCR1B |= (1<<WGM12);	// CTC mode
	OCR1A = TIMER1_TOP;
	OCR1B = TIMER1_TOP;		// Needed since ADC only has a timer1 compare match b trigger source 
	TIMSK1 = 0b00000000;	// No interrupts set 
	
}


// Start timer1 1 prescaler (no prescaling occurring)
void timer1_start(void){
	TCCR1B |= (1<<CS10);
}	


void timer1_stop(void){
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}

// Reset timer1 counter to 0, clear compare b flag to reset
void timer1_reset(void)
{	
	
	TCNT1 = 0;				
	TIFR1 = (1 << OCF1B);	 
}

// Timer2 CTC mode 1024 prescaler interrupt on compare match A
void timer2_init(void)
{
	TCCR2A = (1 << WGM21);								
	OCR2A  = TIMER2_TOP;						
	TIMSK2 = (1 << OCIE2A);								
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);	
}


// Main calls this every loop, returns 1 once per report period 
uint8_t timer2_report_due(void)
{
	if (report_flag) {
		report_flag = 0;	// Clear it so the next call returns 0 until the next period is up
		return 1;
	}
	return 0;
}

// Runs every 99.84 ms, masked during bursts (see capture.c)
ISR(TIMER2_COMPA_vect)
{
	report_ticks++;
	if (report_ticks >= TICKS_PER_REPORT) {
		report_ticks = 0;
		report_flag = 1;
	}
}

