/*
 * main.c
 *
 * Created: 04/22/25 11:47:55
 *  Author: sewel
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>

volatile uint16_t lastCapt = 0, per = 0; setPoint = 0;
volatile uint8_t newPer = 0; override = 0; 

void adc_init()
{
	DDRC = 0x00; // Set ADC as input
	ADMUX = (1 << REFS0); // AVCC reference voltage, ADC0
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (ADPS1) | (1 << ADPS0); // Enable ADC, set prescaler to 128
}

uint16_t adc_read()
{
	ADCSRA |= (1 << ADSC); // Start ADC conversion
	while(ADCSRA & (1 << ADSC)); // Wait for conversion to complete
	return ADC;
}

void pwm0_init()
{
	// Fast PWM, non-inverting on OC0A, prescaler at 64
	TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1);
	TCCR0B = (1 << CS01) | (1 << CS00);
	DDRD |= (1 << PIND6); // PD6 (OC0A) as output
}

void icp1_init()
{
	TCCR1A = 0;
	// Noise cancel, rising edge, prescaler = 8
	TCCR1B = (1 << ICES1) | (1 << ICNC1) | (1 << CS11);
	TIMSK1 = (1 << ICIE1); // ICP interrupt
	sei(); // Enable global interrupts
}

ISR(TIMER1_CAPT_vect)
{
	uint16_t now = ICR1;
	per = now - lastCapt;
	lastCapt = now;
	newPer = 1;
}

void usart_init(uint16_t ubrr)
{
	UBRR0H = ubrr >> 8;
	UBRR0L = ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

ISR(USART0_RX_vect)
{
	char buffer[4];
	uint8_t idx = 0;
	char c = UDR0;
	if (c >= '0' && c <= '9' && idx < 3 )
	{
		buffer[idx++] = c;
	}
	else if ((c == '\r' || c == '\n') && idx > 0)
	{
		buffer[idx] = 0;
		setPoint = (uint16_t)atoi(buffer);
		override = 1;
		idx = 0;
	}
}

void uart_print(uint16_t s, uint16_t m)
{
	char tmp[32];
	int n = snprintf(tmp, sizeof(tmp), "%u,%u\n", s, m);
	for (int i = 0; i < n; i++)
	{
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = tmp[i];
	}
}

int main(void)
{
	uint16_t raw;
	uint8_t duty;
	uint16_t meas_rpm = 0;
	
	// Initialization
	adc_init();
	pwm0_init();
	icp1_init();
	usart_init((F_CPU/16/9600)-1);
	
	// Clear flags
	setPoint = 0;
	override = 0;
	
	for(;;)
	{
		// Compute PWM duty cycle
		if (override)
		{
			duty = (setPoint > 255) ? 255 : (uint8_t)setPoint;
		}
		else
		{
			raw	= adc_read(); // 0 to 1023
			duty = raw >> 2; // Shift to 0 to 255
		}
		
		OCR0A = duty;
		
		// Measure RPM if after new capture
		if (newPer)
		{
			newPer = 0;
			meas_rpm = (uint16_t)((F_CPU/8.0 * 60.0) / per); // Calculate ticks/sec
		}
		
		// Send measurement for plotting
		uart_print(duty, meas_rpm);
		_delay_ms(50); // Print every 50ms
	}
}