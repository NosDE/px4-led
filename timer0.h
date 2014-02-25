#ifndef _TIMER0_H
#define _TIMER0_H

#include <inttypes.h>

#define TIMER_DIVIDER		CLK256
//#define TIMER_RELOAD_VAL	198	// 1kHz @ 14.7456 MHz
#define TIMER_RELOAD_VAL	193 // 1kHz @ 16.000 MHz

extern volatile uint16_t cntMilliSecs;

extern void timer0_Init(void);
extern uint16_t timer0_setDelay (uint16_t t);
extern int8_t timer0_checkDelay (uint16_t t);

#endif
