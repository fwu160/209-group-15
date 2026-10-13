/*
 * zerocross.h
 *
 * Created: 9/23/2026 8:34:40 PM
 *  Author: hiimf
 */ 


#ifndef ZEROCROSS_H_
#define ZEROCROSS_H_
#include "common.h"

void zc_init(void);		// one-time setup: configure INT0 for a rising edge

// Blocks until a real rising edge is detected by INT0 (hardware, not
// polling), or ZC_TIMEOUT_LOOPS is exceeded. Returns 1 on success, 0 on timeout 
uint8_t zc_wait_for_edge(void);




#endif /* ZEROCROSS_H_ */