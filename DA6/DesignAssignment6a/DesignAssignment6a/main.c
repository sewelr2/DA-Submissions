/*
 * main.c
 *
 * Created: 05/15/25 12:55:29
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

//—— USART @9600, TX only ———————————————————————————————————
#define BAUD        9600
#define UBRR_VAL    ((F_CPU/16/BAUD) - 1)

static void USART_init(void)
{
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)UBRR_VAL;
	UCSR0B = (1<<TXEN0);                    // TX enable
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);       // 8N1
}

static void USART_send(char c)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

static void USART_print_u16(uint16_t x)
{
	char buf[6];
	uint8_t i = 0;
	if (x == 0)
	{
		USART_send('0');
		return;
	}
	while (x && i < sizeof(buf))
	{
		buf[i++] = '0' + (x % 10);
		x /= 10;
	}
	while (i--)
	USART_send(buf[i]);
}

//—— Servo (Timer1 Fast PWM Mode?14, prescaler=64 ? 50?Hz) ———————
static void servo_init(void)
{
	SERVO_DDR |= (1<<SERVO_PIN);            // PB1 output

	// COM1A1=1 non?inverting OC1A, WGM11=1
	TCCR1A = (1<<COM1A1)|(1<<WGM11);
	// WGM13=1, WGM12=1, CS11=1, CS10=1 ? prescaler=64, Mode?14
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);

	ICR1 = 4999;                            // TOP = 4999 ? 50?Hz
}

// map 0–180° ? 250–500 ticks (1?ms–2?ms @ 4?µs/tick)
static inline void servo_setAngle(uint8_t angle)
{
	OCR1A = 250 + ((uint32_t)angle * 250) / 180;
}

//—— HC?SR04 setup & measurement by polling TCNT1 —————————————
static void ultrasonic_init(void)
{
	TRIG_DDR  |=  (1<<TRIG_PIN);            // trigger pin output
	DDRD      &= ~(1<<ECHO_PIN);            // echo pin input
	PORTD     &= ~(1<<ECHO_PIN);            // no pull?up
}

static uint16_t ultrasonic_read_raw(void)
{
	uint16_t start, end;

	// 10?µs trigger pulse
	TRIG_PORT |=  (1<<TRIG_PIN);
	_delay_us(10);
	TRIG_PORT &= ~(1<<TRIG_PIN);

	// wait for echo high
	while (!(ECHO_PINR & (1<<ECHO_PIN)));
	start = TCNT1;

	// wait for echo low
	while  (ECHO_PINR &  (1<<ECHO_PIN));
	end = TCNT1;

	return end - start;  // raw ticks (4?µs each)
}

//—— Main sweep —————————————————————————————————————————
int main(void)
{
	USART_init();
	servo_init();
	ultrasonic_init();

	while (1)
	{
		// CW sweep: 0?180 in 2° steps
		for (uint8_t ang = 0; ang <= 180; ang += 2)
		{
			servo_setAngle(ang);
			Wait();

			uint16_t raw = ultrasonic_read_raw();
			USART_print_u16(ang);
			USART_send(',');
			USART_print_u16(raw);
			USART_send('\n');
		}
		// CCW sweep: 180?0
		for (int8_t ang = 180; ang >= 0; ang -= 2)
		{
			servo_setAngle(ang);
			Wait();

			uint16_t raw = ultrasonic_read_raw();
			USART_print_u16(ang);
			USART_send(',');
			USART_print_u16(raw);
			USART_send('\n');
		}
	}
}
