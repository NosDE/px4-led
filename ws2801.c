#include "ws2801.h"


void ws2801_init(void)
{
	PIX_Direction_REG |= (1<<PIX_Clock);		// set clock pin as output
	PIX_Direction_REG |= (1<<PIX_DO);		// set data out pin as output
} // ws2801_init


void ws2801_setPixel(uint8_t red, uint8_t green, uint8_t blue) {
	int i=0;
	for (i=0; i<MAX_PIXEL; i++) {
		ws2801_writeByte(red);
		ws2801_writeByte(green);
		ws2801_writeByte(blue);
	}
} // ws2801_setPixel


void ws2801_setOnePixel(uint8_t red, uint8_t green, uint8_t blue) {
	ws2801_writeByte(red);
	ws2801_writeByte(green);
	ws2801_writeByte(blue);
} // ws2801_setPixel


void ws2801_showPixel(void){
	PIX_Write &= ~(1<<PIX_Clock);	// set clock LOW
	/* a 500uS delay is need to display frame on ws2801. we do this in main */
} // ws2801_showPixel


void ws2801_writeByte(uint8_t Send)
{
	register uint8_t BitCount = 8; // store variable BitCount in a cpu register
	do {
		PIX_Write &= ~(1<<PIX_Clock);	// set clock LOW
		// send bit to ws2801. we do MSB first
		if (Send & 0x80) 
		{
			PIX_Write |= (1<<PIX_DO); // set output HIGH
		} else {
			PIX_Write &= ~(1<<PIX_DO); // set output LOW
		}
		PIX_Write |= (1<<PIX_Clock); // set clock HIGH
		// next bit
		Send <<= 1;
	} while (--BitCount);
} // ws2801_writeByte
