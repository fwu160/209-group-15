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
#define K_V   0.109922f		// Source volts per ADC count
#define K_I   0.004037f		// Load amps per ADC count

// V burst starts this many cycles early to cancel I lag
// About 11.1 cycles/deg on the V burst 
#define PHASE_TRIM_CYCLES   23

#define MIN_SAMPLES_APART	10		// Closer crossings are noise (a cycle is ~20 samples)


// UART send rate
/* 
Timer2 runs in CTC mode, counting 0 ->> 194 (195 counts) = 99.84 ms per
compare match (which is one tick). A UART frame goes out every TICKS_PER_REPORT ticks.

One capture (V burst + I burst + calculation) takes about 80 ms (in theory) and a
UART frame about 25 ms, therefore 5 ticks = 0.5 s -> about 5-6 captures averaged per frame
*/

#define TIMER2_TOP			194		// OCR2A value: 195 counts x 512 us = 99.84 ms
#define TICKS_PER_REPORT    5    


/* Display */ 
#define TIMER0_TOP	79	// Display timer TOP, 80 counts x 32 us = 2.56 ms per digit







#endif /* COMMON_H_ */