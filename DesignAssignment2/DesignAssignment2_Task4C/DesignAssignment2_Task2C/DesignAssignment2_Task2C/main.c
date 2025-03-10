/*
 * main.c
 *
 * Created: 03/09/25 22:45:23
 *  Author: sewel
 */ 

#define F_CPU 16000000UL // set CPU clock freq
#include <avr/io.h>
#include <util/delay.h>

// 150ms delay subroutine
void delay_150ms()
{
	_delay_ms(150);
}

int main(void)
{
	DDRB |= (1 << 5); // Set PB5 as output
	PORTB |= (1 << 5); // LED initially off
	
	DDRC &= (0 << 1); // Set PC1 as input
	PORTC |= (1 << 1); // Enable PC1 pull-up resistor
	
	while(1)
	{
		// Poll for switch press (active low)
		if(!(PINC & (1 << PINC1))) // Check for PC1 input
		{
			PORTB ^= (1 << 5); // Turn on LED at PB5
			// Loop 150ms delay subroutine 10 times for 1.5 sec delay
			for (int i = 0; i < 10; i++) // Loop 10 times
			{
				delay_150ms(); // Call 150ms delay function
			}
			PORTB ^= (1 << 5); // Turn off LED
		}
	}
	
	return 0;
}
