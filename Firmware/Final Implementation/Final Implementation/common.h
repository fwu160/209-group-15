/*
 * common.h
 *
 * Created: 9/7/2026 3:57:35 PM
 *  Author: 
 */ 


#ifndef COMMON_H_
#define COMMON_H_

#include <avr/io.h>
#include <stdint.h>

#define F_CPU 2000000UL
#define BAUD 9600		// UBRR = 12, real baud is 9615 (0.16% off)
#define UBRR ((F_CPU / (16UL * BAUD)) - 1)

#define SAMPLE_RATE_HZ  10000UL		// samples per second, per channel   
#define N_SAMPLES       200			// samples captured per channel/burst
 
 
/* ADC */ 

/* Timer1 top value for CTC mode, no prescaler (clk/1):
 one compare match every (TIMER1_TOP + 1) CPU cycles.
 At 2 MHz / 10 kHz this is 200 cycles -> TOP = 199.
 */ 
#define TIMER1_TOP      ((uint16_t)((F_CPU / SAMPLE_RATE_HZ) - 1))

//  ADC channels
#define ADC_CHANNEL_V   1		// ADC1: conditioned voltage input 
#define ADC_CHANNEL_I   0		// ADC0: conditioned current input 


/* 
How long zc_wait_for_edge() spins before giving up on a missing
zero-cross, in loop iterations (rough estimate just used to know
when to give up on measurement )
*/
#define ZC_TIMEOUT_LOOPS 20000U
 
 
/* Calibration */
/* Converts ADC counts back into real volts & amps, and lines up 
the samples in real time. Finetuned with the scope and multimeter 
K_new = K_old * (meter reading / displayed reading) */

/* K_V is the source volts per ADC count (Vrms = RMS counts * K_V)
 The divider (56k/2.7k) + level-shift op-amp shrink the source
 about (1/ measured gain)times, so 1 count (5/1024 = 4.88 mV 
 at the pin) is K_V at source.*/
#define K_V   0.10942f		    

/* K_I is the load amps per ADC count (Irms = RMS counts * K_I)
 1A through the shunt gives 0.5 V, and the op-amp boosts it
 to about 1.22V at the pin, so 1 count = about K_I of load current.
 Additionally, power = mean(Vcounts x Icounts) * K_V * K_I*/
#define K_I   0.0040104f		

/* the filters delay I slightly more than V, so the V burst 
 starts [number of phase_trim_cycle] CPU cycles early to line them up
 and cancel the I lag. About 11.1 cycles/deg on the V burst */
#define PHASE_TRIM_CYCLES   13 

/* 10 kHz sampling / 500 Hz signal = 20 samples per cycle. Zero crossings
 closer than half a cycle (10 samples) must be noise, so ignore them. */
#define MIN_SAMPLES_APART	10	


// UART send rate
/* 
Timer2 runs in CTC mode, counting 0 ->> 194 (195 counts) = 99.84 ms per
compare match (which is one tick). A UART frame goes out every TICKS_PER_REPORT ticks.

One capture (V burst + I burst + calculation) takes about 80 ms (in theory) and a
UART frame about 25 ms, therefore 5 ticks = 0.5 s -> about 5-6 captures averaged per frame
*/

#define TIMER2_TOP			194		// OCR2A value: 195 counts x 512 us = 99.84 ms
#define TICKS_PER_REPORT    5    

// Time one report covers: 5 x 195 x 1024 / 2 MHz = 0.4992 s
#define REPORT_PERIOD  ((float)TICKS_PER_REPORT * (TIMER2_TOP + 1) * 1024 / F_CPU)


/* Display */ 
#define TIMER0_TOP	79	// Display timer TOP, 80 counts x 32 us = 2.56 ms per digit







#endif /* COMMON_H_ */