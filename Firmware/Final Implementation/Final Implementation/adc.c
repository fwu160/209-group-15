/*
 * adc.c
 *
 * Created: 9/23/2026 8:33:01 PM
 *  Author: hiimf
 */ 

#include <avr/interrupt.h>
#include "adc.h"


// vars that ISR will need to have access to 
static volatile uint16_t *capture_buffer;
static volatile uint16_t  capture_n;
static volatile uint16_t  capture_index;

// flag to tell whatever function that is using adc that the burst capture is done 
static volatile uint8_t   capture_done_flag;

void adc_init(void)
{
	ADMUX =0;				// clear all mux bits to 0 
	ADMUX |= (1<<REFS0);	// Vref to 5V Vcc
	ADCSRA |= (1<<ADPS1)|(1<<ADPS0);	// set to prescaler of 8, should get 125kHz from 2Mhz CPU clk
	ADCSRA |= (1<<ADEN);	// enable adc peripheral
	ADCSRA |= (1<<ADSC);	// do a dummy coversion to warm up peripheral  
	
}

void adc_select_channel(uint8_t channel)
{
	ADMUX &= ~((1<<MUX3)|(1<<MUX2)|(1<<MUX1)|(1<<MUX0)); // clear all mux bits to 0000, ready for update 
	ADMUX |= (channel & 0x0F);	// select new channel, use mask to make sure only 4 right bits are being changed eg 0010 & 00001111 <= mask will become 00000010

	ADCSRA |= (1 << ADSC);       // one ordinary blocking conversion to start up the adc peripheral 
	
	// loop to make sure conversion finishes before anything else uses adc 
	while(ADCSRA & (1<<ADSC)){
		; // wait 
	}
	
}


// Tells the ADC where to store the next n triggered samples and arms auto-triggering + the ADC interrupt.
void adc_start_capture(volatile uint16_t *buffer, uint16_t n)
{
	// this is done so that the ISR can access the variables passed into adc_start_capture 
	capture_buffer = buffer;
	capture_n = n;
	capture_index = 0;
	capture_done_flag = 0;

	// ADTS bits = 101 , Timer/Counter1 Compare Match B is the trigger source
	ADCSRB = (ADCSRB & 0xF8) | (1 << ADTS2) | (0 << ADTS1) | (1 << ADTS0);
	TIFR1  = (1 << OCF1B);   // clear any leftover/stale flag in timer1 flag reg before the first real trigger
	// enable auto trigger, enable adc completed interrupt  
	ADCSRA |= (1 << ADATE) | (1 << ADIE);
}

// return adc burst capture done flag to whatever function is using burst capture 
uint8_t adc_capture_done(void) { 
	return capture_done_flag; 

}

// stop all ADC interrupts, isnt triggered by timer1 anymore
void adc_stop(void)
{
	ADCSRA &= (uint8_t)~((1 << ADATE) | (1 << ADIE));
}

ISR(ADC_vect)
{
	// Must be cleared every sample, or the ADC stops seeing the NEXT Timer1 compare match as a fresh trigger edge 
	// no Timer1 interrupts set so thats why we need to manually clear it every time with = (NOT |= since in datasheet it says set 1 to clear so using |= could acidentally clear other flags in reg)
	TIFR1 = (1 << OCF1B);
	
	// stores ADC value in given capture array 
	capture_buffer[capture_index] = ADC;
	capture_index++;

	// If capture index reaches given array size, stop adding and set the capture done flag to 1 to let function know burst capture is done
	if (capture_index >= capture_n) {
		capture_done_flag = 1;
	}
}

