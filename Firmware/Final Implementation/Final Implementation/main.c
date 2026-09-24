/*
 * Final Implementation.c
 *
 * Created: 9/7/2026 3:55:59 PM
 * Author : 
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>


#include "common.h"
#include "timer.h"
#include "adc.h"
#include "zerocross.h"
#include "capture.h"

#define   RMSVoltage     14.5
#define   PeakCurrent    125
#define   Power          1.60
 
static uint16_t v_buf[N_SAMPLES];
static uint16_t i_buf[N_SAMPLES];

int main(void)
{
	DDRB |= (1 << 0); // pb0 is debug pin

	
	timer1_init();
	adc_init();
	zc_init();
	// uart_init(UBRR); // Initiate uart
	sei();   // enable global interrupts once every module above is configured
	
	
	    while (1){	
			
		// enum from capture.h
		capture_result_t result = capture_phase_locked(v_buf, i_buf);


		if (result == CAPTURE_OK) {
			// TODO once power_calc exists: power_calc_run(v_buf, i_buf, N_SAMPLES, ...)
			
			
			PORTB ^= (1 << PORTB0);   // one toggle per good capture
			} else {
			PORTB ^= (1 << PORTB0);   // double-pulse = timed out
			PORTB ^= (1 << PORTB0);
		}
    }
}

