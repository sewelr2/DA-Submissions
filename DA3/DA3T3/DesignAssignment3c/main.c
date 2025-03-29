/*
 * main.c
 *
 * Created: 03/28/25 13:52:00
 *  Author: sewel
 */ 

#include <xc.h>
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint16_t timer4_count = 0;
#define TIMER4_MAX_COUNT 10000  // 1 sec delay (10000 * 100µs)

ISR(TIMER4_OVF_vect)
{
	TCNT4 = 65336; // Load TCNT4 with 100 us period
	
	timer4_count++;
	if (timer4_count >= TIMER4_MAX_COUNT)
	{
		PORTB ^= (1 << PINB3); // Toggle LED
		timer4_count = 0;
	}
}

int main(void)
{
	DDRB |= (1 << PINB3); // Set PB3 as LED output
	
	TCCR4A = 0; // Timer4 normal mode
	TCCR4B = (1 << CS41); // Start Timer4 with prescaler = 8
	
	TCNT4 = 65336; // Load Timer4 for the first overflow after 100us
	
	TIMSK4 |= (1 << TOIE4); // Enable Timer4 overflow interrupt
	
	sei(); // Enable global interrupts
	
    while(1)
    {

    }
	
	return 0;
}