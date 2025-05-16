#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "font.h"     // your SSD1306oled + font helper

/* === UART === */
#define USART_BAUDRATE  9600
#define UBRR_VALUE      (((F_CPU/(USART_BAUDRATE*16UL)))-1)

void UART_init(void) {
	UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);   // 8 data bits, 1 stop bit
	UCSR0B = (1<<TXEN0);                // enable transmitter
}

void UART_tx(char c) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void UART_str(const char *s) {
	while (*s) UART_tx(*s++);
}

/* === I²C (TWI) === */
void i2c_init(void) {
	TWSR0 = 0x00;
	TWBR0 = ((F_CPU/100000UL) - 16) / 2;
	TWCR0 = (1<<TWEN);
}

void i2c_start(void) {
	TWCR0 = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while (!(TWCR0 & (1<<TWINT)));
}

void i2c_stop(void) {
	TWCR0 = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	_delay_us(10);
}

void i2c_write(uint8_t data) {
	TWDR0 = data;
	TWCR0 = (1<<TWINT)|(1<<TWEN);
	while (!(TWCR0 & (1<<TWINT)));
}

uint8_t i2c_read_ack(void) {
	TWCR0 = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while (!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

uint8_t i2c_read_nack(void) {
	TWCR0 = (1<<TWINT)|(1<<TWEN);
	while (!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

/* === BMI160 === */
#define BMI160_ADDR    0x68
#define BMI160_CMD_REG 0x7E
#define BMI160_ACC_REG 0x12
#define BMI160_GYR_REG 0x0C

void bmi160_write(uint8_t reg, uint8_t val) {
	i2c_start();
	i2c_write((BMI160_ADDR<<1)|0);
	i2c_write(reg);
	i2c_write(val);
	i2c_stop();
}

void bmi160_read(uint8_t reg, uint8_t *buf, uint8_t len) {
	i2c_start();
	i2c_write((BMI160_ADDR<<1)|0);
	i2c_write(reg);
	_delay_us(10);
	i2c_start();
	i2c_write((BMI160_ADDR<<1)|1);
	for (uint8_t i = 0; i < len-1; i++) {
		buf[i] = i2c_read_ack();
	}
	buf[len-1] = i2c_read_nack();
	i2c_stop();
}

void bmi160_init(void) {
	// Soft-reset
	bmi160_write(BMI160_CMD_REG, 0xB6);
	_delay_ms(100);
	// Normal accel mode
	bmi160_write(BMI160_CMD_REG, 0x11);
	_delay_ms(50);
	// Normal gyro mode
	bmi160_write(BMI160_CMD_REG, 0x15);
	_delay_ms(50);
}

void bmi160_accel(int16_t *x, int16_t *y, int16_t *z) {
	uint8_t d[6];
	bmi160_read(BMI160_ACC_REG, d, 6);
	*x = (int16_t)((d[1]<<8) | d[0]);
	*y = (int16_t)((d[3]<<8) | d[2]);
	*z = (int16_t)((d[5]<<8) | d[4]);
}

void bmi160_gyro(int16_t *x, int16_t *y, int16_t *z) {
	uint8_t d[6];
	bmi160_read(BMI160_GYR_REG, d, 6);
	*x = (int16_t)((d[1]<<8) | d[0]);
	*y = (int16_t)((d[3]<<8) | d[2]);
	*z = (int16_t)((d[5]<<8) | d[4]);
}

/* === OLED via font.h (renamed to ssd1306oled_*) === */
void oled_init(void) {
	ssd1306oled_init();
	ssd1306oled_clear();
}

void oled_display_angles(float roll, float pitch, float yaw) {
	char buf[32];
	ssd1306oled_clear();

	// Line 0: Roll
	ssd1306oled_set_cursor(0, 0);
	snprintf(buf, sizeof(buf), "Roll: %+5.1f", roll);
	ssd1306oled_write_string(buf);

	// Line 1: Pitch
	ssd1306oled_set_cursor(0, 1);
	snprintf(buf, sizeof(buf), "Pitch:%+5.1f", pitch);
	ssd1306oled_write_string(buf);

	// Line 2: Yaw
	ssd1306oled_set_cursor(0, 2);
	snprintf(buf, sizeof(buf), "Yaw:  %+5.1f", yaw);
	ssd1306oled_write_string(buf);

	ssd1306oled_update();
}

/* === Main Loop === */
int main(void) {
	// Initialize peripherals
	i2c_init();
	UART_init();
	oled_init();
	_delay_ms(100);
	bmi160_init();
	UART_str("BMI160 + OLED Ready\r\n");

	// Sensor and filter variables
	int16_t ax, ay, az, gx, gy, gz;
	float f_ax, f_ay, f_az;
	float roll = 0, pitch = 0, yaw = 0;
	const float dt  = 0.1f;         // 100 ms loop
	const float R2D = 57.2957795f;  // rad ? deg
	char uart_buf[32];

	while (1) {
		// 1) Read raw sensor data
		bmi160_accel(&ax, &ay, &az);
		bmi160_gyro (&gx, &gy, &gz);

		// 2) Convert and compute angles
		f_ax = ax/16384.0f;  f_ay = ay/16384.0f;  f_az = az/16384.0f;
		roll  = atan2f( f_ay,      f_az) * R2D;
		pitch = atan2f(-f_ax, sqrtf(f_ay*f_ay + f_az*f_az)) * R2D;
		yaw  += (gz/262.4f) * dt;
		if (yaw >  180) yaw -= 360;
		if (yaw < -180) yaw += 360;

		// 3) UART output
		snprintf(uart_buf, sizeof(uart_buf),
		"R:%+5.1f P:%+5.1f Y:%+5.1f\r\n",
		roll, pitch, yaw);
		UART_str(uart_buf);

		// 4) OLED update
		oled_display_angles(roll, pitch, yaw);

		_delay_ms(100);
	}
	return 0;
}
