/*
 * common.h
 *
 * Created: 9/7/2026 3:57:35 PM
 *  Author: 
 */ 


#ifndef COMMON_H_
#define COMMON_H_

#define F_CPU 2000000UL
#define BAUD 9600
#define UBRR ((F_CPU / (16UL * BAUD)) - 1)

#define SAMPLE_RATE_HZ  10000UL    // samples per second, per channel   
#define N_SAMPLES       200        // samples captured per channel/burst
 
/* Timer1 top value for CTC mode, no prescaler (clk/1):
 
 one compare match every (TIMER1_TOP + 1) CPU cycles.
 At 2 MHz / 10 kHz this is 200 cycles -> TOP = 199.
 */ 

#define TIMER1_TOP      ((uint16_t)((F_CPU / SAMPLE_RATE_HZ) - 1))

/* ---- ADC channels ---- */
#define ADC_CHANNEL_V   1   // ADC1: conditioned voltage input 
#define ADC_CHANNEL_I   0   // ADC0: conditioned current input 

/* ---- Zero-cross input pin (comparator output -> INT0) ---- */
#define ZC_DDR          DDRD
#define ZC_PIN_NUM      PD2

/* How long zc_wait_for_edge() spins before giving up on a missing
 zero-cross, in loop iterations (rough estimate just used to know
 when to give up on measurement )
  */
#define ZC_TIMEOUT_LOOPS 20000U
 

#define VOLTS_PER_COUNT  (5.0f / 1024.0f)      // ADC: 5V reference, 10-bit
#define K_V   (VOLTS_PER_COUNT * 14.1f)        // source volts per count NEED TO CHANGE AFTER CALABRATION
#define K_I   (VOLTS_PER_COUNT * 2.0f)         // load amps per count    NEED TO CHANGE AFTER CALABRATION

/* UART send rate
Timer2 runs in CTC mode from the 2 MHz clock / 1024 = 1953.125 Hz,
so one count = 512 us. Counting 0 ->> 194 (195 counts) = 99.84 ms per
compare match (which is one tick). A UART frame goes out every TICKS_PER_REPORT ticks.

One capture (V burst + I burst + calculation) takes about 80 ms (in theory) and a
UART frame about 25 ms, therefore 5 ticks = 0.5 s -> about 5-6 captures averaged per frame
*   2 ticks = 0.2 s -> about 2 captures averaged per frame
*   3 ticks = 0.3 s -> about 3-4 captures averaged per frame
*   5 ticks = 0.5 s -> about 5-6 captures averaged per frame   <- chosen 


*/


#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>




#endif /* COMMON_H_ */