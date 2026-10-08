/*
 * calculation.h
 *
 * Created: 9/28/2026 12:43:55 PM
 *  Author: hiimf
 */ 


#ifndef CALCULATION_H_
#define CALCULATION_H_

#include "common.h"

typedef struct {
	float   vrms;      // volts, RMS 
	float   irms;      // amps, RMS, technically not needed but easy to calc so might be useful later
	float   ipeak_ma;  // milliamps, peak
	float   power;     // watts, real power
	uint8_t clipped;    // 1 if any raw sample hit 0 or 1023 - result is unreliable
} power_result_t;

// Takes the two raw ADC-count buffers from one phase-locked capture
// and fills `result` with real-world Vrms, Irms, peak current, and
// real power. Pure computation - no register access, no hardware
// dependency, no state carried between calls - so it can be compiled
// and tested on a PC as well as on the AVR.
void power_calc_run(const uint16_t *v_buf, const uint16_t *i_buf, power_result_t *result);




#endif /* CALCULATION_H_ */