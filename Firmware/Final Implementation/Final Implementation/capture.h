/*
 * capture.h
 *
 * Created: 9/23/2026 11:04:54 PM
 *  Author: hiimf
 */ 


#ifndef CAPTURE_H_
#define CAPTURE_H_
#include "common.h"
// enum used to know result state of current capture
typedef enum {
	CAPTURE_OK = 0,
	CAPTURE_ERR_NO_ZC_V,
	CAPTURE_ERR_NO_ZC_I
} capture_result_t;

capture_result_t capture_phase_locked(volatile uint16_t *v_buffer, volatile uint16_t *i_buffer);



#endif /* CAPTURE_H_ */