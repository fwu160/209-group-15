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
	float   vrms;      // Volts, RMS 
	float   irms;      // Amps, RMS, technically not needed but easy to calc so might be useful later
	float   ipeak_ma;  // Milliamps, peak
	float   power;     // Watts, real power
	float   phase_deg; // Degrees, phase difference between voltage and current
	uint8_t clipped;    // 1 if any raw sample hit 0 or 1023 then result is unreliable
} reading_t;

 
//Fills result with Vrms, Irms, peak current and power from one capture, phase is left at 0, power_avg_get() does it 
void power_calc_run(const uint16_t *v_buf, const uint16_t *i_buf, reading_t *result);

//How many samples from start to end of a buffer that make up a whole number of cycles 
uint16_t whole_cycle_samples(const uint16_t *v_buf);

// running average of several captures
typedef struct {
	float   vrms_sum;
	float   irms_sum;
	float   ipeak_ma_sum;
	float   power_sum;
	uint8_t count;      // how many captures have been added since the last reset
	uint8_t clipped;    // 1 if ANY of them clipped
} reading_sums_t;

void power_avg_reset(reading_sums_t *avg);                                  // Empty power avg (after each UART frame)
void power_avg_add(reading_sums_t *avg, const reading_t *one);         // Add one capture result to power avg
void power_avg_get(const reading_sums_t *avg, reading_t *result);     // The average (only call if count > 0)



#endif /* CALCULATION_H_ */