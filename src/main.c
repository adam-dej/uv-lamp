/*
 * main.c
 *
 *  Created on: Apr 7, 2013
 *      Author: deykoweec
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define sbi(PORT, PIN) PORT |= (1 << PIN)
#define cbi(PORT, PIN) PORT &= ~(1 << PIN)

#define ON 1
#define OFF 0

void init_io();
void _blank_display();
void init_timers();
void main_timer_start();
void main_timer_stop();
void blink_timer_start();
void blink_timer_stop();
void enable_INT0();
void disable_INT0();
void enable_INT1();
void disable_INT1();
void lamp(int state);
void init_ext_interrupts();

uint8_t digit = 0;
uint8_t minute = 0;
uint8_t blink = 0;
uint8_t digit_edit = 0;
uint8_t lamp_state = 0;

int number = 0;

ISR(INT0_vect) {
	if (digit_edit == 0) digit_edit = 1;
	else digit_edit = 0;
	disable_INT0();
}

ISR(INT1_vect) {
	disable_INT1();
	if (minute == 0 && number == 0 && lamp_state == 1) {
		lamp(OFF);
		lamp_state = 0;
	} else if (minute == 0 && number == 0 && lamp_state == 0) {
		lamp(ON);
		lamp_state = 1;
	} else {
		if (lamp_state == 0) {
			lamp(ON);
			lamp_state = 1;
			main_timer_start();
			blink_timer_stop();
		} else {
			lamp(OFF);
			lamp_state = 0;
			main_timer_stop();
			blink_timer_start();
		}
	}
}

ISR(TIMER1_COMPB_vect) {
	if (number > 0) number--;
	else {
		if (minute > 0) {
			number = 59;
			minute--;
		} else {
			main_timer_stop();
		}
	}
	if (number == 0 && minute == 0) {
		lamp(OFF);
		lamp_state = 0;
		blink_timer_start();
	}
	TCNT1 = 0;
}

ISR(TIMER2_OVF_vect) {
	if (blink < 10) blink++;
	else blink = 0;
}

ISR(TIMER0_OVF_vect) {

	uint8_t d = 0;
	_blank_display();

	switch(digit) {
	case 0:
		cbi(PORTC, 5);
		if (!(digit_edit == 0 && blink < 1)) sbi(PORTD, 5);
		d = number/10;
		break;
	case 1:
		cbi(PORTD, 5);
		sbi(PORTD, 6);
		d = number%10;
		break;
	case 2:
		cbi(PORTD, 6);
		if (!(digit_edit == 1 && blink < 1)) sbi(PORTC, 5);
		PORTB = ~(minute << 1);
		break;
	}

	if (digit < 2) {
		digit++;
		if (d != 4 && d != 1) cbi(PORTB, 5);
		if (d != 5 && d != 6) cbi(PORTB, 4);
		if (d != 2) cbi(PORTB, 3);
		if (d != 1 && d != 4 && d != 7) cbi(PORTB, 2);
		if (d == 2 || d == 6 || d == 0 || d == 8) cbi(PORTB, 1);
		if ((d > 3 && d != 7) || d == 0) cbi(PORTD, 7);
		if (d != 1 && d != 7 && d != 0) cbi(PORTB, 0);
	}
	else digit = 0;



}


int main(void) {

	init_io();
	init_ext_interrupts();
	init_timers();

	minute = 0;
	number = 0;

	lamp(OFF);
	enable_INT0();
	enable_INT1();

	blink_timer_start();

	while(1) {
		if (!bit_is_set(PIND, 0) && ((digit_edit == 0 && number < 60) | (digit_edit == 1 && minute < 31)) && !bit_is_set(TCCR1B, CS12)) {
			if (digit_edit == 0) number += 10;
			else minute++;
			_delay_ms(200);
			while(!bit_is_set(PIND, 0)) {
				_delay_ms(100);
				if (!bit_is_set(PIND, 0) && ((digit_edit == 0 && number < 60) | (digit_edit == 1 && minute < 31))) {
					if (digit_edit == 0) number += 10;
					else minute++;
				}
			}
		}
		if (!bit_is_set(PIND, 1) && ((digit_edit == 0 && number >= 10) | (digit_edit == 1 && minute > 0)) && !bit_is_set(TCCR1B, CS12)) {
			if (digit_edit == 0) number -= 10;
			else minute--;
			_delay_ms(200);
			while (!bit_is_set(PIND, 1)) {
				_delay_ms(100);
				if (!bit_is_set(PIND, 1) && ((digit_edit == 0 && number >= 10) | (digit_edit == 1 && minute > 0))) {
					if (digit_edit == 0) number -= 10;
					else minute--;
				}
			}
		}
		if (!bit_is_set(GICR, INT0)) {
			_delay_ms(500);
			GIFR |= (1 << INTF0);
			enable_INT0();
		}
		if (!bit_is_set(GICR, INT1)) {
			_delay_ms(500);
			GIFR |= (1 << INTF1);
			enable_INT1();
		}
	}

	while(1);


}

void init_io() {
	sbi(DDRB, 5);
	sbi(DDRB, 4);
	sbi(DDRB, 3);
	sbi(DDRB, 2);
	sbi(DDRB, 1);
	sbi(DDRB, 0);
	sbi(DDRD, 7);
	sbi(DDRD, 6);
	sbi(DDRD, 5);

	sbi(DDRD, 4);

	cbi(DDRD, 0);
	cbi(DDRD, 1);
	sbi(PORTD, 0);
	sbi(PORTD, 1);
}

void _blank_display() {
	sbi(PORTB, 5);
	sbi(PORTB, 4);
	sbi(PORTB, 3);
	sbi(PORTB, 2);
	sbi(PORTB, 1);
	sbi(PORTB, 0);
	sbi(PORTD, 7);
}

void init_timers() {
	TIMSK |= (1 << TOIE0);
	TIMSK |= (1 << TOIE2);
	TIMSK |= (1 << OCIE1B);
	TCNT0 = 0;
	OCR1B = 31250; //Compare every second with 8MHz and /256 prescaler
	sei();
	TCCR0 |= (1 << CS01) | (1 << CS00);
}

void blink_timer_start() {
	TCCR2 |= (1 << CS22) | (1 << CS21) | (1 << CS20);
}

void blink_timer_stop() {
	TCCR2 &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
	blink = 5;
}

void main_timer_start() {
	TCCR1B |= (1 << CS12);
}

void main_timer_stop() {
	TCCR1B &= ~(1 << CS12);
}

void init_ext_interrupts() {
	cbi(DDRD, 2);
	cbi(DDRD, 3);
	sbi(PORTD, 2);
	sbi(PORTD, 3);
	MCUCR |= (1 << ISC11) | (1 << ISC01);
}

void enable_INT0() {
	GICR |= (1 << INT0);
}

void disable_INT0() {
	GICR &= ~(1 << INT0);
}

void enable_INT1() {
	GICR |= (1 << INT1);
}

void disable_INT1() {
	GICR &= ~(1 << INT1);
}

void lamp(int state) {
	if (state) sbi(PORTD, 4);
	else cbi(PORTD, 4);
}
