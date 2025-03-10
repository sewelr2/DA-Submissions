/*
 * main.c
 *
 * Created: 3/9/2025 22:59:21
 *  Author: milit
 */ 

#define F_CPU 16000000UL // set CPU clock freq
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// 150ms delay subroutine
void delay_150ms()
{
	_delay_ms(150);
}

int main(void)
{
	DDRB |= (1 << 5); // Set PB5 as output
	PORTB |= (1 << 5); // LED initially off
	
	DDRD &= (0 << 2); // Set PD2 as input
	PORTD |= (1 << 2); // Enable PD2 pull-up
	
	// Configure INT0 interrupt
	EICRA |= (1 << ISC01) | (1 << ISC00); // Trigger on rising edge
	EIMSK |= (1 << INT0); // Enable INT0
	
	sei(); // Enable global interrupts
	
	while(1)
	{

	}
	
	return 0;
}

ISR(INT0_vect)
{
	PORTB ^= (1 << 5); // Turn on PB5 LED
	// Loop 150ms delay subroutine 20 times for 3 sec delay
	for(int i = 0; i < 20; i++) // Loop 20 times
	{
		delay_150ms(); // Call 150ms delay subroutine
	}
	PORTB ^= (1 << 5); // Turn off LED
}
