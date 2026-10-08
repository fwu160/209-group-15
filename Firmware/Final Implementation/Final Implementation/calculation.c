/*
 * calculation.c
 *
 * Created: 9/28/2026 12:43:44 PM
 *  Author: hiimf
 */ 

#include <math.h>
#include "calculation.h"


void power_calc_run(const uint16_t *v_buf, const uint16_t *i_buf, power_result_t *result){
	
		// used to sum up all the adc values in array
		uint32_t sum_v = 0;
		uint32_t sum_i = 0;
		
		// for loop to sum up all values in array 
		for (uint16_t k = 0; k < N_SAMPLES; k++) {
				sum_v += v_buf[k];
				sum_i += i_buf[k];
		}
		
		// get offset value of v and i bursts 
		uint16_t offset_v = (uint16_t)(sum_v / N_SAMPLES);
		uint16_t offset_i = (uint16_t)(sum_i / N_SAMPLES);
		
		
		int32_t  sum_v2 = 0;		// running sum of (v-deviation)^2
		int32_t  sum_i2 = 0;		// running sum of (i-deviation)^2
		int32_t  sum_vi = 0;		// running sum of (v-deviation * i-deviation)
		uint16_t peak_i_adc = 0; // largest |i-deviation| seen so far (adc count, not converted)
		
		uint8_t  clipped = 0;		// flag to check if adc values at rail (it should never be at rail due to the conditioning done to signal)

		
		
		for (uint16_t k = 0; k < N_SAMPLES; k++) {
			// if function checks if any of the adc values is at rail 0->0V or 1023->5V
			if (v_buf[k] == 0 || v_buf[k] == 1023 || i_buf[k] == 0 || i_buf[k] == 1023) {
						clipped = 1;
					}
			
			int16_t dv = (int16_t)v_buf[k] - (int16_t)offset_v;
			int16_t di = (int16_t)i_buf[k] - (int16_t)offset_i;
			
			
			// Get all the squared values and sum them up 
			sum_v2 += (int32_t)dv * dv;
			sum_i2 += (int32_t)di * di;
			sum_vi += (int32_t)dv * di;
				
				
			// find max deviation from 0 of the i samples, max deviation will be peak i 
			uint16_t abs_di;
			if (di < 0) {
				abs_di = (uint16_t)(-di);   // negative: flip the sign to make it positive
				} else {
				abs_di = (uint16_t)di;      // already zero or positive: keep as-is
			}			
			// update peak i 
			if (abs_di > peak_i_adc) {
				peak_i_adc = abs_di;
			}
			}
			float mean_sq_v = (float)sum_v2 / (float)N_SAMPLES;
			float mean_sq_i = (float)sum_i2 / (float)N_SAMPLES;
			
			float vrms_counts = sqrtf(mean_sq_v);   // RMS voltage, still in raw ADC counts
			float irms_counts = sqrtf(mean_sq_i);   // RMS current, still in raw ADC counts

			result->vrms      = vrms_counts * K_V;
			result->irms      = irms_counts * K_I;
			result->ipeak_ma  = (float)peak_i_adc * K_I * 1000.0f;  
	
			float mean_vi = (float)sum_vi / (float)N_SAMPLES;
			result->power = mean_vi * K_V * K_I;
			
			result->clipped = clipped;
			

		
		
		
		
		
		


		
		
		
	
}