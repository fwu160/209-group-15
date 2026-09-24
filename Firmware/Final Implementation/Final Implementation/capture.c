/*
 * capture.c
 *
 * Created: 9/23/2026 11:05:03 PM
 *  Author: hiimf
 */ 
#include "capture.h"
#include "timer.h"
#include "adc.h"
#include "zerocross.h"

// One burst on one channel, will be used to capture V and I signals from the same relative point (rising zero crossing on V signal)
static uint8_t capture_one_burst(volatile uint16_t *buffer, uint8_t channel)
{
	adc_select_channel(channel);      
	timer1_reset();
	adc_start_capture(buffer, N_SAMPLES);  // start adc capture

	if (!zc_wait_for_edge()) {          // real INT0 interrupt underneath; timer1_start()
		adc_stop();                     // already happened inside that ISR if it succeeded
		return 0;
	}
	// block loop here to wait for adc burst to finish, might need to remove if main would need to do something 
	// but burst capture should have number 1 priorty as accuracy is most important 
	while (!adc_capture_done()) {
		;
	}

	timer1_stop();
	adc_stop();
	return 1;
}

capture_result_t capture_phase_locked(volatile uint16_t *v_buffer, volatile uint16_t *i_buffer)
{
	// if capture_one_burst for v fails it returns 0, then return CAPTURE_ERR_NO_ZC_V error code
	if (!capture_one_burst(v_buffer, ADC_CHANNEL_V)) {
		return CAPTURE_ERR_NO_ZC_V;
	}
	
	// if capture_one_burst for i fails it returns 0, then return CAPTURE_ERR_NO_ZC_I error code

	if (!capture_one_burst(i_buffer, ADC_CHANNEL_I)) {
		return CAPTURE_ERR_NO_ZC_I;
	}
	
	
	return CAPTURE_OK;
}