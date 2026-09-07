/*
 * common.h
 *
 * Created: 9/7/2026 3:57:35 PM
 *  Author: 
 */ 


#ifndef COMMON_H_
#define COMMON_H_

#define F_CPU 2000000UL
#define BAUD 9600
#define UBRR ((F_CPU / (16UL * BAUD)) - 1)


#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>




#endif /* COMMON_H_ */