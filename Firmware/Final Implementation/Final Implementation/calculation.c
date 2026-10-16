/*
 * calculation.c
 *
 * Created: 9/28/2026 12:43:44 PM
 *  Author: hiimf
 */ 

#include <math.h>
#include "calculation.h"

/*
Find how many samples make whole cycles, using the voltage samples
Every time the voltage crosses its middle value going UP, one cycle has passed. The spacing between those crossings is samples per cycle
doing this instead of using interrupts to keep track of what sample count at zero cross since no delays incurred during adc sampling due to interrupts
*/

// works out how many samples (from the start of the buffer) make up a whole number of cycles,
// so the rms/power calcs don't include a leftover part of a cycle when the source isn't exactly 500Hz
uint16_t whole_cycle_samples(const uint16_t *v_buf)
{
	// Average of all the voltage samples, used as the "zero" line of the wave
	uint32_t sum = 0;
	for (uint16_t k = 0; k < N_SAMPLES; k++) {
		sum += v_buf[k];
	}
	int16_t mid = (int16_t)(sum / N_SAMPLES);

	float first = 0.0f;			// Where the first rising crossing is (in samples, can be a fraction)
	float last  = 0.0f;			// Where the latest rising crossing is
	uint8_t crossings = 0;		// Number of rising crossings found

	// Go through the samples looking for where the wave goes from below mid to above mid, rising zero cross 
	for (uint16_t k = 1; k < N_SAMPLES; k++) {
		int16_t before = (int16_t)v_buf[k - 1] - mid;	// Signed so it can go negative
		int16_t after  = (int16_t)v_buf[k] - mid;
		
		// Find the before and after samples where zero cross occurs
		if (before < 0 && after >= 0) {
			// Crossing is somewhere between sample k-1 and k, draw a straight line between them to find exactly where it hits zero, float used 
			float position = (float)(k - 1) + (float)(-before) / (float)(after - before);

			if (crossings == 0) {
				// first one found
				first = position;
				last = position;
				crossings = 1;
				// only count it if it's at least half a cycle after the last one, otherwise it's just noise
				} else if (position - last >= MIN_SAMPLES_APART) {
				
				last = position;
				crossings++;
			}
		}
	}

	// Need at least 2 crossings to measure a cycle, if not just use the whole buffer like before
	if (crossings < 2) {
		return N_SAMPLES;
	}

	// Distance from first to last crossing divided by the number of cycles between them
	float samples_per_cycle = (last - first) / (float)(crossings - 1);		// ~19.6 at 510Hz, 20 at 500Hz, 20.4 at 490Hz

	// How many whole cycles fit in the buffer (the +0.5 stops noise turning exactly 10 cycles into 9)
	uint16_t cycles = (uint16_t)(((float)N_SAMPLES + 0.5f) / samples_per_cycle);

	// Turn that back into a number of samples, round to nearest whole value
	uint16_t n = (uint16_t)((float)cycles * samples_per_cycle + 0.5f);

	// Rounding could push it 1 over, can't use more samples than we have
	if (n > N_SAMPLES) {
		n = N_SAMPLES;
	}
	return n;
}
void reading_calc_run(const uint16_t *v_buf, const uint16_t *i_buf, reading_t *result){
		
		uint16_t n = whole_cycle_samples(v_buf);

		// Used to sum up all the adc values in array
		uint32_t sum_v = 0;
		uint32_t sum_i = 0;
		
		// For loop to sum up all values in array 
		for (uint16_t k = 0; k < n; k++) {
				sum_v += v_buf[k];
				sum_i += i_buf[k];
		}
		
		// Get offset value of v and i bursts 
		uint16_t offset_v = (uint16_t)(sum_v / n);
		uint16_t offset_i = (uint16_t)(sum_i / n);
		
		
		int32_t  sum_v2 = 0;		// Sum of (v-deviation)^2
		int32_t  sum_i2 = 0;		// Sum of (i-deviation)^2
		int32_t  sum_vi = 0;		// Sum of (v-deviation * i-deviation)
		uint16_t peak_i_adc = 0; // largest |i-deviation| seen so far (adc count, not converted)
		
		uint8_t  clipped = 0;		// flag to check if adc values at rail (it should never be at rail due to the conditioning done to signal)

		
		
		for (uint16_t k = 0; k < n; k++) {
			// If function checks if any of the adc values is at rail 0->0V or 1023->5V
			if (v_buf[k] == 0 || v_buf[k] == 1023 || i_buf[k] == 0 || i_buf[k] == 1023) {
						clipped = 1;
			}
			
			int16_t dv = (int16_t)v_buf[k] - (int16_t)offset_v;
			int16_t di = (int16_t)i_buf[k] - (int16_t)offset_i;
			
			
			// Get all the squared values and sum them up 
			sum_v2 += (int32_t)dv * dv;
			sum_i2 += (int32_t)di * di;
			sum_vi += (int32_t)dv * di;
				
				
			// Find max deviation from 0 of the i samples, max deviation will be peak i 
			uint16_t abs_di;
			if (di < 0) {
				abs_di = (uint16_t)(-di);   // Negative: flip the sign to make it positive
				} else {
				abs_di = (uint16_t)di;      // Already zero or positive: keep as-is
			}			
			// Update peak i 
			if (abs_di > peak_i_adc) {
				peak_i_adc = abs_di;
			}
			
		}
		
		// Mean squared of V and I adc counts
		float mean_sq_v = (float)sum_v2 / (float)n;
		float mean_sq_i = (float)sum_i2 / (float)n;
			
		float vrms_counts = sqrtf(mean_sq_v);		// RMS voltage, still in raw ADC counts
		float irms_counts = sqrtf(mean_sq_i);		// RMS current, still in raw ADC counts

		// Put values into result pointer 
		result->vrms      = vrms_counts * K_V;		// V RMS with calibration const 
		result->irms      = irms_counts * K_I;		// I RMS with calibration const 
		result->ipeak_ma  = (float)peak_i_adc * K_I * 1000.0f;  // Peak i with calibration const 
	
		float mean_vi = (float)sum_vi / (float)n;
		result->power = mean_vi * K_V * K_I;
		
		result->phase_deg = 0.0f;	// Do phase calc when transmitting, saves calc time

		result->clipped = clipped;	// Clipped flag 
		
	
}

// Averaging several captures

static float phase_from_power(float power, float vrms, float irms)
{
	float apparent = vrms * irms;		// Volt-amps
	float power_factor = 1.0f;			// Default 1 p.f, 0 deg if there's no current
	if (apparent > 0.0f) {
		power_factor = power / apparent;
	}
	// Noise can push it past +-1, acos would give erorr
	if (power_factor > 1.0f) {
		power_factor = 1.0f;
	}
	if (power_factor < -1.0f) {
		power_factor = -1.0f;
	}
	return acosf(power_factor) * 57.29578f;	
}

void reading_avg_reset(reading_sums_t *avg)
{
	avg->vrms_sum     = 0.0f;
	avg->irms_sum     = 0.0f;
	avg->ipeak_ma_sum = 0.0f;
	avg->power_sum    = 0.0f;
	avg->count        = 0;
	avg->clipped      = 0;
}

void reading_avg_add(reading_sums_t *avg, const reading_t *one)
{
	avg->vrms_sum     += one->vrms;
	avg->irms_sum     += one->irms;
	avg->ipeak_ma_sum += one->ipeak_ma;
	avg->power_sum    += one->power;
	avg->count++;
	if (one->clipped) {
		avg->clipped = 1;
	}
}

void reading_avg_get(const reading_sums_t *avg, reading_t *result)
{
	float n = (float)avg->count;

	result->vrms     = avg->vrms_sum / n;
	result->irms     = avg->irms_sum / n;
	result->ipeak_ma = avg->ipeak_ma_sum / n;
	result->power    = avg->power_sum / n;
	result->clipped  = avg->clipped;

	// phase from the averaged power, voltage and current (steadier than averaging angles)
	result->phase_deg = phase_from_power(result->power, result->vrms, result->irms);
}