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
 * one compare match every (TIMER1_TOP + 1) CPU cycles.
 * At 2 MHz / 10 kHz this is 200 cycles -> TOP = 199.
 * (was F_CPU_HZ, which is never defined anywhere - that's a silent
 * bug: the preprocessor would refuse to compile this the moment
 * anything actually used TIMER1_TOP.) */
#define TIMER1_TOP      ((uint16_t)((F_CPU / SAMPLE_RATE_HZ) - 1))

/* ---- ADC channels ---- */
#define ADC_CHANNEL_V   0   // ADC0 = conditioned voltage input 
#define ADC_CHANNEL_I   1   // ADC1 = conditioned current input 

/* ---- Zero-cross input pin (comparator output -> INT0) ---- */
#define ZC_DDR          DDRD
#define ZC_PIN_NUM      PD2

/* How long zc_wait_for_edge() spins before giving up on a missing
 * zero-cross, in loop iterations (not milliseconds - it's a cheap
 * loop counter, not a hardware timer, so this is approximate). */
#define ZC_TIMEOUT_LOOPS 20000U



#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>




#endif /* COMMON_H_ */