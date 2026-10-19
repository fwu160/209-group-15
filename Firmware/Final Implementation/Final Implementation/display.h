/*
 * display.h
 *
 * Created: 12/10/2026 3:45:56 pm
 *  Author: fwu160
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_


#include "common.h"
#include "timer.h"

// pins on our PCB
#define SH_CP_BIT    0      // PE0 - shift clock
#define SH_DS_BIT    3      // PD3 - data
#define SH_ST_BIT    4      // PD4 - latch
#define OE_BIT       5      // PD5 - output enable (active low) = OC0B
#define SRCLR_BIT    6      // PD6 - clear (active low), held high
#define DS1_BIT      2      // PB2 - digit 1 (leftmost), active low
#define DS2_BIT      1      // PB1 - digit 2
#define DS3_BIT      0      // PB0 - digit 3
#define DS4_BIT      7      // PD7 - digit 4 (rightmost)

#define DISPLAY_MIN_BLANK 6		// Min OCR0B, 224 us dark on each digit change

#define DISPLAY_BRIGHTNESS_DEFAULT  50   // percent, used by display_init()

void display_init(void);                       // call before sei()
void display_set_brightness(uint8_t percent);  // 0 = off, 1-100 = dim to full


void display_show_vrms(float volts);		// Shows U13.3
void display_show_ipeak(float milliamps);	// Shows C282
void display_show_power(float watts);		// Shows P5.37


#endif /* DISPLAY_H_ */



