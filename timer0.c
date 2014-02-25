#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer0.h"

enum {
	STOP		= 0,
	CLK		= 1,
	CLK8		= 2,
	CLK64		= 3,
	CLK256		= 4,
	CLK1024		= 5,
	FALLING_EDGE	= 6,
	RISING_EDGE	= 7
};


volatile uint16_t cntMilliSecs = 0;


void timer0_Init(void)
{
	uint8_t sreg = SREG;
	cli();
	TCCR0B = TIMER_DIVIDER;
	TCNT0 = TIMER_RELOAD_VAL;
	TIMSK0 |= (1 << TOIE0);
	cntMilliSecs = 0;
	SREG = sreg;
	sei();
}


uint16_t timer0_setDelay (uint16_t t)
{
	return(cntMilliSecs + t - 1);
}


int8_t timer0_checkDelay(uint16_t t)
{
	return(((t - cntMilliSecs) & 0x8000)>>8);
}


ISR(TIMER0_OVF_vect)
{
	cntMilliSecs++;
	TCNT0 = TIMER_RELOAD_VAL;
}
