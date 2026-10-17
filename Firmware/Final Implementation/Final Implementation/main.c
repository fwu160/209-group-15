/*
 * Final Implementation.c
 *
 * Created: 9/7/2026 3:55:59 PM
 * Author : 
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>          // dtostrf() lives here
#include <string.h>  



#include "common.h"
#include "display.h"
#include "timer.h"
#include "adc.h"
#include "zerocross.h"
#include "capture.h"
#include "calculation.h"
#include "uart.h"

// Reports per value on the display (2 = 1sec)
#define REPORTS_PER_DISPLAY_ITEM   2


// Which value goes on the display next
typedef enum {
	SHOW_VRMS = 0,
	SHOW_IPEAK,
	SHOW_POWER,
	SHOW_COUNT		// Number of values, used to wrap around
} display_item_t;
// A reading with every field 0, used when there's no signal
static const reading_t NO_SIGNAL = {0};
 
static uint16_t v_buf[N_SAMPLES];
static uint16_t i_buf[N_SAMPLES];


float energy = 0;	// Energy measured in Joules, measured from power on 

static void send_results(const reading_t *p, float energy)
{
	char num[16];

	dtostrf(p->vrms, 1, 2, num);         // Two decimal places -> "13.81"
	uart_transmit_string(num);
	uart_transmit(',');

	dtostrf(p->ipeak_ma, 1, 0, num);     // whole mA -> "1992"
	uart_transmit_string(num);
	uart_transmit(',');

	dtostrf(p->power, 1, 2, num);        // 2 decimal places -> "24.40"
	uart_transmit_string(num);
	
	uart_transmit(',');
	
	dtostrf(p->phase_deg, 1, 2, num);    // phase difference, 2 decimal place -> "36.92"
	uart_transmit_string(num);
	
	uart_transmit(',');
	
	dtostrf(energy, 1, 1, num);			// Energy in J, 1 decimal place -> "1234.5"
	uart_transmit_string(num);
	
	
	uart_transmit_string("\r\n");
	
	
}

// Shows one value from the latest result
static void show_on_display(display_item_t item, const reading_t *p)
{
	if (item == SHOW_VRMS) {
		display_show_vrms(p->vrms);
		} else if (item == SHOW_IPEAK) {
		display_show_ipeak(p->ipeak_ma);
		} else {
		display_show_power(p->power);
	}
}


int main(void)
{
	
	
	DDRB |= (1 << DDB5); // pb5 is debug pin
	reading_sums_t avg;          // running average of the captures since the last UART frame
	reading_t latest = {0};					// Last averaged reading result, zeros = no signal
	display_item_t disp_item = SHOW_VRMS;			// What goes on the display next
	uint8_t frames = REPORTS_PER_DISPLAY_ITEM - 1;	// Start at 1 so the display updates on the first report
	
	
	timer1_init();
	timer2_init();   
	adc_init();
	zc_init();
	uart_init(UBRR);
	display_init();
	reading_avg_reset(&avg);

	sei();		// Enable global interrupts once every module above is configured
	
	
	while (1){	
			
		// enum from capture.h
		capture_result_t result = capture_phase_locked(v_buf, i_buf);


		if (result == CAPTURE_OK) {
			reading_t one;
			reading_calc_run(v_buf, i_buf, &one);   // whole cycles only (inside the calculation)
			reading_avg_add(&avg, &one);            // add to the running average - no sending here any more
			
			
		} 

		
		// Timer2 says the report period is up -> send the average of everything captured since the last frame
		if (timer2_report_due()) {
			if (avg.count > 0) {
				reading_avg_get(&avg, &latest);
				
				}
				else {
				latest = NO_SIGNAL;	// No good capture, send and show zeros
			}
			
			if (latest.power >0){
				energy += latest.power * REPORT_PERIOD;
			}
			send_results(&latest, energy);
			
			// Every 1 s show the next value
			frames++;
			if (frames >= REPORTS_PER_DISPLAY_ITEM) {
				frames = 0;
				show_on_display(disp_item, &latest);
				disp_item++;
				if (disp_item >= SHOW_COUNT) {
					disp_item = SHOW_VRMS;
				}
			}
			
		reading_avg_reset(&avg);    // start a fresh average for the next period
		
		
		}
	}
}

