/*
 * adc.h
 *
 * Created: 9/23/2026 8:33:15 PM
 *  Author: hiimf
 */ 


#ifndef ADC_H_
#define ADC_H_

#include "common.h"

void adc_init(void);
void adc_select_channel(uint8_t channel); 

void adc_start_capture(volatile uint16_t *buf, uint16_t n);

uint8_t adc_capture_done(void);  
void adc_stop(void);



#endif /* ADC_H_ */