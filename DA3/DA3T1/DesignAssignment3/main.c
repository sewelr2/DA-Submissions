/*
 * main.c
 *
 * Created: 03/27/25 10:34:15
 *  Author: sewel
 */ 

#define F_CPU 16000000UL
#include <xc.h>
#include <avr/io.h>
#include <util/delay.h>

#define DELAY_125US_COUNT 12000 // 1.5 sec delay

int main(void)
{
	DDRB |= (1 << PINB5); //Set PB5 as LED output
	
	
    while(1)
    {
		// Turn on LED
        PORTB |= (1 << PINB5);
		for (uint16_t i = 0; i < DELAY_125US_COUNT; i++)
		{
			TCNT0 = 6; // Load 6 into TCNT0 to count 250 ticks
			TIFR0 |= (1 << TOV0); // Clear timer0 overflow flag
			TCCR0B = (1 << CS01); // Start Timer0 with prescaler = 8
			// Wait for 125 us
			while (!(TIFR0 & (1 << TOV0)));
		}
		
		PORTB &= ~(1 << PINB5); // Turn off LED
		for (uint16_t i = 0; i < DELAY_125US_COUNT; i++)
		{
			TCNT0 = 6; // Load 6 into TCNT0 to count 250 ticks
			TIFR0 |= (1 << TOV0); // Clear timer0 overflow flag
			TCCR0B = (1 << CS01); // Start Timer0 with prescaler = 8
			// Wait for 125 us
			while (!(TIFR0 & (1 << TOV0)));
		}
    }
	
	return 0;
}