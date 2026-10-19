/*
 * capture.h
 *
 * Created: 10/10/2026 10:20:12 PM
 *  Author: hiimf
 */ 

#include <avr/interrupt.h>
#include "display.h"

static volatile uint8_t disp_characters[4] = {0, 0, 0, 0};  // pattern for each digit
static volatile uint8_t disp_pos = 0;                       // digit lit next

#define SEG_DP  0b10000000      // Bit 7 is the DP

// Decimals for each value, the number always uses the right 3 digits
#define VRMS_DECIMALS   1       // U13.3
#define IPEAK_DECIMALS  0       // C282
#define POWER_DECIMALS  2       // P5.37

// Letters for the leftmost digit (bit 0 = A ... bit 6 = G)
#define SEG_U  0b00111110       // B C D E F, U for voltage since V can't be drawn
#define SEG_C  0b00111001       // A D E F
#define SEG_P  0b01110011       // A B E F G

// segment patterns for 0-9 on 7 seg (bit 7 is decimal point)
static const uint8_t seg_pattern[10] = {
	0b00111111, 
	0b00000110, 
	0b01011011, 
	0b01001111, 
	0b01100110,
	0b01101101, 
	0b01111101, 
	0b00000111, 
	0b01111111, 
	0b01101111
};



void display_init(void)
{
	// Digits off SRCLR high OE high (disp off), everything else low
	PORTB |= (1 << DS1_BIT) | (1 << DS2_BIT) | (1 << DS3_BIT);
	PORTD |= (1 << DS4_BIT) | (1 << SRCLR_BIT) | (1 << OE_BIT);
	DDRB  |= (1 << DS1_BIT) | (1 << DS2_BIT) | (1 << DS3_BIT);
	DDRD  |= (1 << SH_DS_BIT) | (1 << SH_ST_BIT) | (1 << OE_BIT) | (1 << SRCLR_BIT) | (1 << DS4_BIT);
	DDRE  |= (1 << SH_CP_BIT);

	timer0_init();
	display_set_brightness(DISPLAY_BRIGHTNESS_DEFAULT);
}

// 0 = off, 1 = dimmest, 100 = brightest.
void display_set_brightness(uint8_t percent)
{
	
	// If percent is 0 then disp off
	if (percent == 0) {
		TCCR0A &= ~(1 << COM0B1);   // disconnect OE from the PWM
		PORTD |= (1 << OE_BIT);     // OE high, all off 
		return;
	}
	// Just in case I enter over 100 
	if (percent > 100) {
		percent = 100;
	}

	
	
	// Setting PWM percentage
	OCR0B = (TIMER0_TOP - 1) - ((uint16_t)(percent - 1) * (TIMER0_TOP - 1 - DISPLAY_MIN_BLANK)) / 99;
	
	TCCR0A |= (1 << COM0B1);	// OE pin controlled by pwm 
}

// Shows a letter on the leftmost digit and the value on the right 3 digits, zeros are kept
static void display_show_fixed(float value, uint8_t decimals, uint8_t letter)
{
	// Scale to a whole number, e.g. 14.48 -> 1448
	float scale = 1.0f;
	for (uint8_t k = 0; k < decimals; k++) {
		scale *= 10.0f;
	}
	
	uint16_t max = 999;		// Biggest number that fits in 3 digits

	
	// Just in case noise makes value go negative 
	if (value < 0.0f) {
		value = 0.0f;
	}
	
	// Clamp before casting so it can't wrap, round value
	float scaled = value * scale + 0.5f;
	uint16_t n;
	if (scaled > max) {
		n = max;
		} else {
		n = (uint16_t)scaled;
	}
	
	uint8_t d[4];
	d[0] = letter;	// Leftmost digit shows the letter
	d[1] = seg_pattern[(n / 100) % 10];
	d[2] = seg_pattern[(n / 10) % 10];
	d[3] = seg_pattern[n % 10];
	

	// DP on the units digit, none if decimal is 0
	if (decimals > 0 && decimals < 4) {
		d[3 - decimals] |= SEG_DP;
	}
	
	TIMSK0 &= ~(1 << TOIE0);	// Only mask the display ISR while copying
	disp_characters[0] = d[0];
	disp_characters[1] = d[1];
	disp_characters[2] = d[2];
	disp_characters[3] = d[3];
	TIMSK0 |= (1 << TOIE0);		// Turn disp interrupt back on
}


void display_show_vrms(float volts) {
	display_show_fixed(volts, VRMS_DECIMALS, SEG_U);
}

void display_show_ipeak(float milliamps) {
	display_show_fixed(milliamps, IPEAK_DECIMALS, SEG_C);
}

void display_show_power(float watts) {
	display_show_fixed(watts, POWER_DECIMALS, SEG_P);
}



// OE should be high when overflow, disp off, ISR_NOBLOCK re-enables interrupts straight away
// so INT0 and the ADC interrupt still active
ISR(TIMER0_OVF_vect, ISR_NOBLOCK)
{
	uint8_t pattern = disp_characters[disp_pos];

	// all digits off
	PORTB |= (1 << DS1_BIT) | (1 << DS2_BIT) | (1 << DS3_BIT);
	PORTD |= (1 << DS4_BIT);

	// shift out 8 bits, MSB first
	for (uint8_t i = 0; i < 8; i++) {
		if (pattern & 0x80) {
			PORTD |= (1 << SH_DS_BIT);
		} else {
			PORTD &= ~(1 << SH_DS_BIT);
		}
		PORTE |= (1 << SH_CP_BIT);
		PORTE &= ~(1 << SH_CP_BIT);
		pattern <<= 1;
	}

	// latch
	PORTD |= (1 << SH_ST_BIT);
	PORTD &= ~(1 << SH_ST_BIT);

	// turn on this digit
	if (disp_pos == 0) {
		PORTB &= ~(1 << DS1_BIT);
	} else if (disp_pos == 1) {
		PORTB &= ~(1 << DS2_BIT);
	} else if (disp_pos == 2) {
		PORTB &= ~(1 << DS3_BIT);
	} else {
		PORTD &= ~(1 << DS4_BIT);
	}

	disp_pos = (disp_pos + 1) & 3;   
}