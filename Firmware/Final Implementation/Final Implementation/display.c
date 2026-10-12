/*
 * display.c
 */

#include <avr/interrupt.h>
#include "display.h"


// segment patterns for 0-9 (bit 0 = A ... bit 6 = G, bit 7 = decimal point)
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

static volatile uint8_t disp_characters[4] = {0, 0, 0, 0};  // pattern for each digit, 0 = leftmost
static volatile uint8_t disp_pos = 0;                       // digit lit next

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

	
	OCR0B = (DISPLAY_TOP - 1) - ((uint16_t)(percent - 1) * (DISPLAY_TOP - 1 - DISPLAY_MIN_BLANK)) / 99;

	TCCR0A |= (1 << COM0B1);	// OE now controled by pwm 
}

// Shows power with 2 decimal places, e.g. 4.25 W -> " 4.25". Range 0.00 to 99.99.
void display_show_power(float watts)
{
	if (watts < 0) {
		watts = 0;
	}
	uint16_t n = (uint16_t)(watts * 100.0f + 0.5f);   // 4.25 -> 425
	if (n > 9999) {
		n = 9999;
	}

	uint8_t d0 = (n >= 1000) ? seg_pattern[n / 1000] : 0;   // blank the leading zero
	uint8_t d1 = seg_pattern[(n / 100) % 10] | 0b10000000;  // units digit + decimal point
	uint8_t d2 = seg_pattern[(n / 10) % 10];
	uint8_t d3 = seg_pattern[n % 10];

	
	TIMSK0 &= ~(1 << TOIE0);	// Turn off only the display interrupt while copying (other interrupts can still run because they are time sensitive) 
	disp_characters[0] = d0;
	disp_characters[1] = d1;
	disp_characters[2] = d2;
	disp_characters[3] = d3;
	TIMSK0 |= (1 << TOIE0);		// Turn disp interrupt back on 
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