/*
 * main.c
 *
 * Created: 05/15/25 12:58:26
 *  Author: sewel
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

//—— your Wait() from reference (~20?ms) ———————————————————————
void Wait(void)
{
	uint8_t i;
	for (i = 0; i < 50; i++)
	{
		_delay_loop_2(0);
		_delay_loop_2(0);
		_delay_loop_2(0);
	}
}

//—— hardware definitions ————————————————————————————————————
// Servo PWM on PB1/OC1A (Timer1)
#define SERVO_DDR   DDRB
#define SERVO_PIN   PINB1

// HC?SR04 Trigger on PC1
#define TRIG_DDR    DDRC
#define TRIG_PORT   PORTC
#define TRIG_PIN    PINC1

// HC?SR04 Echo on PD6
#define ECHO_PINR   PIND
#define ECHO_PIN    PIND6

// USART0 TX only @9600?baud
#define BAUD        9600
#define UBRR_VAL    ((F_CPU/16/BAUD) - 1)

// SPI pins for MAX7219 (7?seg driver)
#define SPI_DDR     DDRB
#define SPI_PORT    PORTB
#define SPI_MOSI    PINB3
#define SPI_SCK     PINB5
#define SPI_SS      PINB2

//—— SPI0 + MAX7219 routines ———————————————————————————————
void SPI_init(void)
{
	SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SPI_PORT |= (1<<SPI_SS);
	SPCR0 = (1<<SPE0)|(1<<MSTR0)|(1<<SPR00);  // SPI0 enabled, Master, Fosc/16
}

void max7219_send(uint8_t reg, uint8_t data)
{
	SPI_PORT &= ~(1<<SPI_SS);
	SPDR0 = reg;
	while (!(SPSR0 & (1<<SPIF0)));
	SPDR0 = data;
	while (!(SPSR0 & (1<<SPIF0)));
	SPI_PORT |= (1<<SPI_SS);
}

void max7219_init(void)
{
	max7219_send(0x09, 0x0F);  // decode mode: digits 0–3
	max7219_send(0x0A, 0x0F);  // intensity
	max7219_send(0x0B, 0x03);  // scan limit: 4 digits
	max7219_send(0x0C, 0x01);  // normal operation
	max7219_send(0x0F, 0x00);  // display test: off
}

static const uint16_t pow10[4] = {1, 10, 100, 1000};

void displayNumber(uint16_t num)
{
	for (uint8_t d = 0; d < 4; d++)
	{
		uint8_t val = (num / pow10[d]) % 10;
		max7219_send(d + 1, val);
	}
}

//—— USART0 TX only ———————————————————————————————————————
void USART_init(void)
{
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)UBRR_VAL;
	UCSR0B = (1<<TXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void USART_send(char c)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void USART_print_u16(uint16_t x)
{
	char buf[6];
	uint8_t i = 0;
	if (x == 0) { USART_send('0'); return; }
	while (x && i < sizeof(buf))
	{
		buf[i++] = '0' + (x % 10);
		x /= 10;
	}
	while (i--) USART_send(buf[i]);
}

//—— Servo (Timer1 Fast PWM Mode?14, prescaler=64 ? 50?Hz) ———————
void servo_init(void)
{
	SERVO_DDR |= (1<<SERVO_PIN);
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);
	ICR1 = 4999;
}

static inline void servo_setAngle(uint8_t angle)
{
	OCR1A = 250 + ((uint32_t)angle * 250) / 180;
}

//—— HC?SR04 setup & measurement by polling TCNT1 —————————————
void ultrasonic_init(void)
{
	TRIG_DDR  |=  (1<<TRIG_PIN);
	DDRD      &= ~(1<<ECHO_PIN);
	PORTD     &= ~(1<<ECHO_PIN);
}

uint16_t ultrasonic_read_raw(void)
{
	uint16_t start, end;
	TRIG_PORT |=  (1<<TRIG_PIN);
	_delay_us(10);
	TRIG_PORT &= ~(1<<TRIG_PIN);

	while (!(ECHO_PINR & (1<<ECHO_PIN)));
	start = TCNT1;
	while  (ECHO_PINR &  (1<<ECHO_PIN));
	end   = TCNT1;

	return end - start;
}

//—— Main sweep with 7?SEG display —————————————————————————
int main(void)
{
	USART_init();
	SPI_init();
	max7219_init();
	servo_init();
	ultrasonic_init();

	while (1)
	{
		uint16_t min_raw = 0xFFFF;

		// CW: display each raw reading
		for (uint8_t ang = 0; ang <= 180; ang += 2)
		{
			servo_setAngle(ang);
			Wait();

			uint16_t raw = ultrasonic_read_raw();
			if (raw < min_raw) min_raw = raw;

			// log on USART
			USART_print_u16(ang);
			USART_send(',');
			USART_print_u16(raw);
			USART_send('\n');

			// show current reading on 7?seg
			displayNumber(raw);
		}

		// CCW: display lowest reading from CW scan
		for (int8_t ang = 180; ang >= 0; ang -= 2)
		{
			servo_setAngle(ang);
			Wait();
			displayNumber(min_raw);
		}
	}
}
