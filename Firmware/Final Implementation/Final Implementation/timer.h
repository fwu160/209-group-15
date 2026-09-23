/*
 * timer.h
 *
 * Created: 9/23/2026 8:34:04 PM
 *  Author: hiimf
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include "common.h"
void timer1_init(void);		// CTC mode, TOP = TIMER1_TOP, left stopped
void timer1_start(void);	// function to start timer1 
void timer1_stop(void);		// function to stop timer1
void timer1_reset(void);	// used to reset timer1 counter and reset all flags




#endif /* TIMER_H_ */