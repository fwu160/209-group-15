/*
 * Final Implementation.c
 *
 * Created: 9/7/2026 3:55:59 PM
 * Author : 
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>          // dtostrf() lives here



#include "common.h"
#include "timer.h"
#include "adc.h"
#include "zerocross.h"
#include "capture.h"
#include "calculation.h"
#include "uart.h"


#define   RMSVoltage     14.5
#define   PeakCurrent    125
#define   Power          1.60
 
static uint16_t v_buf[N_SAMPLES];
static uint16_t i_buf[N_SAMPLES];

static void send_results(const power_result_t *p)
{
	char num[16];

	dtostrf(p->vrms, 1, 1, num);         // 1 decimal place  -> "20.0"
	uart_transmit_string(num);
	uart_transmit(',');

	dtostrf(p->ipeak_ma, 1, 0, num);     // whole mA         -> "1992"
	uart_transmit_string(num);
	uart_transmit(',');

	dtostrf(p->power, 1, 2, num);        // 2 decimal places -> "24.40"
	uart_transmit_string(num);
	uart_transmit_string("\r\n");
}


int main(void)
{
	DDRB |= (1 << 0); // pb0 is debug pin

	
	timer1_init();
	adc_init();
	zc_init();
	uart_init(UBRR); // Initiate uart
	sei();   // enable global interrupts once every module above is configured
	
	
	    while (1){	
			
		// enum from capture.h
		capture_result_t result = capture_phase_locked(v_buf, i_buf);


		if (result == CAPTURE_OK) {
			// TODO once power_calc exists: power_calc_run(v_buf, i_buf, N_SAMPLES, ...)
			
			
			power_result_t p;
			power_calc_run(v_buf, i_buf, &p);
			
			send_results(&p);    // instead of transmit_uart_frame(...)

		
			
			PORTB ^= (1 << PORTB0);   // one toggle per good capture
			} else {
			PORTB ^= (1 << PORTB0);   // double-pulse = timed out
			PORTB ^= (1 << PORTB0);
		}
    }
}

