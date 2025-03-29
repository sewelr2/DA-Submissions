/*
 * main.c
 *
 * Created: 03/27/25 11:00:07
 *  Author: sewel
 */ 

#include <xc.h>
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

volatile uint16_t timer3_count = 0;
#define TIMER3_MAX_COUNT 8000 // 2 sec delay

ISR(TIMER3_COMPA_vect)
{
	timer3_count++;
	if (timer3_count >= TIMER3_MAX_COUNT)
	{
		PORTB ^= (1 << PINB4); // Toggle LED
		timer3_count = 0;
	}
}

int main(void)
{
	DDRB |= (1 << PINB4); // Set PB4 as LED output
	
	// Configure Timer3 for CTC mode
	TCCR3A = 0; // Normal operation
	TCCR3B = (1 << WGM32); // CTC mode
	
	OCR3A = 499; // Prescaler set to 8, 500 ticks needed
	TCCR3B |= (1 << CS31); // Start Timer3, prescaler = 8
	
	TIMSK3 |= (1 << OCIE3A); // Enable Timer3 compare
	
	sei(); // Enable global interrupts
	
    while(1)
    {
        // Interrupt loop controls LED
    }
	
	return 0;
}