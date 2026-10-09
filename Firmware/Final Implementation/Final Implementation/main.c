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
	
	uart_transmit(',');
	
	dtostrf(p->phase_deg, 1, 1, num);    // phase difference, 1 decimal place -> "36.9"
	uart_transmit_string(num);
	
	uart_transmit_string("\r\n");
}


int main(void)
{
	
	
	DDRB |= (1 << 5); // pb5 is debug pin
	power_avg_t avg;          // running average of the captures since the last UART frame
	
	timer1_init();
	timer2_init();   
	adc_init();
	zc_init();
	uart_init(UBRR); // Initiate uart
	power_avg_reset(&avg);

	sei();   // enable global interrupts once every module above is configured
	
	
	    while (1){	
			
		// enum from capture.h
		capture_result_t result = capture_phase_locked(v_buf, i_buf);


		if (result == CAPTURE_OK) {
			power_result_t one;
			power_calc_run(v_buf, i_buf, &one);   // whole cycles only (inside the calculation)
			power_avg_add(&avg, &one);            // add to the running average - no sending here any more
			
			PORTB ^= (1 << PORTB5);   // one toggle per good capture
			} else {
			PORTB ^= (1 << PORTB5);   // double-pulse = timed out
			PORTB ^= (1 << PORTB5);
		}
		
		// Timer2 says the report period is up -> send the average of everything captured since the last frame
		if (timer2_report_due()) {
			if (avg.count > 0) {
				power_result_t p;
				power_avg_get(&avg, &p);
				send_results(&p);
				} else {
				// no good capture this period (no zero crossing seen). Send all zeros so the ESP32
				// still gets a frame - "0.0,0,0.00,0.0" on the dashboard means UART is fine but there is no zero-cross signal.
				uart_transmit_string("0.0,0,0.00,0.0\r\n");
				}
		power_avg_reset(&avg);    // start a fresh average for the next period
		}
    }
}

