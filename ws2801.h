#ifndef _WS2801_H_
#define _WS2801_H_

#include <avr/io.h>

#define PIX_Write			PORTB		// I/O Port
#define PIX_Direction_REG		DDRB		// I/O Port direction register
#define PIX_DO				1		// Data pin of ws2801
#define PIX_Clock			0		// Clock pin of ws2801

#define MAX_PIXEL 10

//Prototypes
extern void ws2801_init(void);
extern void ws2801_setPixel(uint8_t red, uint8_t green, uint8_t blue);
extern void ws2801_setOnePixel(uint8_t red, uint8_t green, uint8_t blue);
extern void ws2801_showPixel(void);
extern void ws2801_writeByte(uint8_t Send);

#endif //_WS2801_H_
