#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define F_CPU 16000000UL
#define BAUD 9600

#define BAUD_PRESCALER ((F_CPU / (16UL * BAUD)) - 1)

// Global variable to store the latest ADC reading
volatile uint16_t adc_value;

void USART_Init()
{
	UBRR0H = (uint8_t)(BAUD_PRESCALER >> 8);
	UBRR0L = (uint8_t)(BAUD_PRESCALER);
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (3 << UCSZ00);
}

void USART_Send(unsigned char data)
{
	// Wait for empty transmit buffer
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void USART_putstring(char* StringPtr)
{
	while(*StringPtr != 0x00)
	{
		USART_Send(*StringPtr);
		StringPtr++;
	}
}

void ADC_Init(void)
{
	ADMUX = (1 << REFS0); // Set AVCC as Vref
	
	// Enable ADC, enable auto-trigger, enable ADC interrupt, set prescaler to 128
	ADCSRA = (1 << ADEN) | (1 << ADATE) | (1 << ADIE) |	(1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	ADCSRA |= (1 << ADTS1) | (1 << ADTS0); // Set ADC auto trigger source to Timer0
	
	ADCSRA |= (1 << ADSC); // Start ADC conversion
}

void Timer1_Init()
{
	TCCR1B = (1 << WGM12); // Set Timer1 to CTC mode
	
	OCR1A = 2499; // 4us tick time for 10ms period = 2500 counts
	
	OCR1B = 2499; // OCR1B used to trigger ADC every 10ms
	
	TCCR1B |= (1 << CS11) | (1 << CS10); // Start Timer1
}

ISR(ADC_vect)
{
	adc_value = ADC; // Read 10-bit ADC result
	
	float voltage = (adc_value * 5.0) / 1024.0; // Convert ADC value to voltage
	
	char buffer[10]; // Holds 10-bit ADC value
	sprintf(buffer, "%.1fV\r\n", voltage); // Format terminal output
	
	USART_putstring(buffer); //  Send voltage value over UART
}

int main(void)
{
	USART_Init(BAUD_PRESCALER); // Initialize UART
	
	ADC_Init(); // Initialize ADC to read PC0
	
	Timer1_Init(); // Initialize Timer1
	
	sei(); // Enable global interrupts
	
	while (1)
	{
		// Wait for ADC ISR to trigger
	}
	
	return 0;
}
