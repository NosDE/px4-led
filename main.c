/*********************************************************************************************
*
* TCA 62724 LED Driver Emulator with WS2801 Bridge and PWM Support
* -----------------------------------------------------------------
*
* TWI Address = 0xaa
*
* Hardware : Arduino Pro with 16MHz
* -----------------------------------------------------------------
* PB5 -> Arduino LED (Status LED)
*
* PC4 -> SDA (TWI)
* PC5 -> SCL (TWI)
*
* PB0 -> SCK (WS2801)
* PB1 -> DO (WS2801)
*
* PD2 -> PWM1 (RGB-LED - blue)
* PB3 -> PWM2 (RGB-LED - green)
* PB4 -> PWM3 (RGB-LED - red)
*
* Important:
* -----------------------------------------------------------------
* Connection to PixHawk (I2C-Port) should be done by SDA & SCL & GND (+5V not connected !!)
* 
* The WS2801 Strip should get its own Power Supply. Don't use power from PixHawk Ports !!!!
*
*********************************************************************************************/

#include <util/twi.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <util/delay.h>
#include "twislave.h"
#include "ws2801.h"
#include "timer0.h"


/*********************************************************************************************
* Set own TWI Address
*********************************************************************************************/
#define SLAVE_ADDRESS 0xaa


/*********************************************************************************************
* Status LED
*********************************************************************************************/
#define STATUS_LED_PORT			PORTB					// i/o port
#define STATUS_LED_DIRECTION_REG	DDRB					// i/o port direction register
#define STATUS_LED			PORTB5					// i/o pin
#define STATUS_LED_INIT			STATUS_LED_DIRECTION_REG |= (1<<STATUS_LED)
#define STATUS_LED_ON			STATUS_LED_PORT |= (1<<STATUS_LED)
#define STATUS_LED_OFF			STATUS_LED_PORT &= ~(1<<STATUS_LED)
#define STATUS_LED_TOGGLE		STATUS_LED_PORT ^= (1<<STATUS_LED)

/*********************************************************************************************
* RGB-LED (3ch pwm)
*********************************************************************************************/
#define PWM_LED_PORT			PORTD					// i/o port
#define PWM_LED_DIRECTION_REG		DDRD					// i/o port direction register
#define PWM_LED_R			PORTD4					// i/o pin (red)
#define PWM_LED_G			PORTD3					// i/o pin (green)
#define PWM_LED_B			PORTD2					// i/o pin (blue)
#define PWM_LED_INIT			PWM_LED_DIRECTION_REG |= (1<<PWM_LED_R) | (1<<PWM_LED_G) | (1<<PWM_LED_B) 

#define PWM_LED_R_ON			PWM_LED_PORT |= (1<<PWM_LED_R)
#define PWM_LED_G_ON			PWM_LED_PORT |= (1<<PWM_LED_G)
#define PWM_LED_B_ON			PWM_LED_PORT |= (1<<PWM_LED_B)

#define PWM_LED_R_OFF			PWM_LED_PORT &= ~(1<<PWM_LED_R)
#define PWM_LED_G_OFF			PWM_LED_PORT &= ~(1<<PWM_LED_G)
#define PWM_LED_B_OFF			PWM_LED_PORT &= ~(1<<PWM_LED_B)


/*********************************************************************************************
* TCA62724 compatible commands
*********************************************************************************************/
#define CMD_ADDR_PWM0			0x81	// blue without auto-increment)
#define CMD_ADDR_PWM1			0x82	// green (without auto-increment)
#define CMD_ADDR_PWM2			0x83	// red (without auto-increment)
#define CMD_ADDR_SETTINGS		0x84	// settings (without auto-increment)
#define CMD_ADDR_PWM_AUTOINCREMENT	0x01	// auto-increment
// Extended Commandset (additional commands)
#define CMD_ADDR_SCRIPT			0xaa	// call auto-script
// Static defines
#define SETTING_POWERSAVE_MASK		0x01	// powersave off
#define SETTING_ENABLE_MASK		0x02	// powersave on


static uint8_t color_r;
static uint8_t color_g;
static uint8_t color_b;
static uint8_t replybuffer[2];
static uint8_t powersave		= 0;
static uint8_t enable			= 0;
static uint8_t script_id		= 0;
static uint16_t scripttimer		= 0;
static uint16_t looptimer		= 0;
static uint16_t ledBlinkTimer		= 0;
static uint8_t softPWM_Pulse_Counter	= 0;

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

void colorChange(void)
{
	static uint8_t colorChangeStage=0;

	if(timer0_checkDelay(scripttimer)) {

		switch (colorChangeStage) {
		case 0:
			ws2801_setPixel(255, 0, 0);
			ws2801_showPixel();
			colorChangeStage++;
			break;
		case 1:
			ws2801_setPixel(0, 255, 0);
			ws2801_showPixel();
			colorChangeStage++;
			break;
		case 2:
			ws2801_setPixel(0, 0, 255);
			ws2801_showPixel();
			colorChangeStage++;
			break;
		case 3:
			ws2801_setPixel(255, 255, 0);
			ws2801_showPixel();
			colorChangeStage++;
			break;
		case 4:
			ws2801_setPixel(0, 255, 255);
			ws2801_showPixel();
			colorChangeStage++;
			break;
		case 5:
			ws2801_setPixel(255, 0, 255);
			ws2801_showPixel();
			colorChangeStage++;
			break;
		case 6:
			ws2801_setPixel(255, 255, 255);
			ws2801_showPixel();
			colorChangeStage = 0;
			break;
		}

		scripttimer = timer0_setDelay(500); // 500ms delay
	}
}


void colorChaser(void)
{
	static uint8_t colorChaserStage=0;

	if(timer0_checkDelay(scripttimer)) {

		switch (colorChaserStage) {
		case 0:
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_showPixel();
			colorChaserStage++;
			break;
		case 1:
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_showPixel();
			colorChaserStage++;
			break;
		case 2:
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(255, 0, 0);
			ws2801_setOnePixel(0, 0, 255);
			ws2801_showPixel();
			colorChaserStage = 0;
			break;
		}
      
		scripttimer = timer0_setDelay(100); // 100ms delay
	}
}


static void build_reply(uint8_t volatile *output_buffer_length, volatile uint8_t *output_buffer,
		uint8_t reply_length, const uint8_t *reply_string)
{
	uint8_t ix;
	output_buffer[0] = reply_length;
	for(ix = 0; ix < reply_length; ix++) {
		output_buffer[1 + ix] = reply_string[ix];
	}
	*output_buffer_length = reply_length + 1;
}

static void fill_replybuffer(void)
{
	uint8_t byte0 = 0;
	uint8_t byte1 = 0;
      
	byte0 = (powersave * SETTING_POWERSAVE_MASK) + (enable * SETTING_ENABLE_MASK);
	byte0 = ((byte0 & 0x0f) << 4) + (color_b >> 4);
	byte1 = ((color_g >> 4) << 4) + (color_r >> 4);
	replybuffer[0] = byte0;
	replybuffer[1] = byte1;
}

static void idle_callback(void)
{
	if(timer0_checkDelay(looptimer)) {
		fill_replybuffer();
		if (script_id > 0) {
			switch (script_id) {
			case 1:
				colorChaser();
				break;
			case 2:
				colorChange();
				break;
			}
		} else {
			ws2801_setPixel(color_r, color_g, color_b);
			ws2801_showPixel();
		}
      
		looptimer = timer0_setDelay(10); // 10ms delay
	}

	if (timer0_checkDelay(ledBlinkTimer)) {
		STATUS_LED_TOGGLE;
		ledBlinkTimer = timer0_setDelay(500); // 500ms delay
	}
}

static void twi_callback(volatile uint8_t input_buffer_length, const volatile uint8_t *input_buffer,
						uint8_t volatile *output_buffer_length, volatile uint8_t *output_buffer)
{
	uint8_t	command;
	uint8_t i=0;

	if(input_buffer_length < 1) {
		fill_replybuffer();
		return(build_reply(output_buffer_length, output_buffer, sizeof(replybuffer), replybuffer));
	}
	
	command	= input_buffer[0];
	switch(command) {
	case CMD_ADDR_SETTINGS:
		powersave = input_buffer[1] & 0x01;
		enable = input_buffer[1] & 0x02;
		break;
	case CMD_ADDR_PWM0:
	case CMD_ADDR_PWM1:
	case CMD_ADDR_PWM2:
		for (i=0; i<input_buffer_length; i++) {
			if (input_buffer[i] == CMD_ADDR_PWM0) {
				color_b = input_buffer[i+1] << 4;
			} else if (input_buffer[i] == CMD_ADDR_PWM1) {
				color_g = input_buffer[i+1] << 4;
			} else if (input_buffer[i] == CMD_ADDR_PWM2) {
				color_r = input_buffer[i+1] << 4;
			}
			
			i++;
		}
		break;
	case CMD_ADDR_PWM_AUTOINCREMENT:
		if (input_buffer_length == 4) {
			color_b = input_buffer[1] << 4;
			color_g = input_buffer[2] << 4;
			color_r = input_buffer[3] << 4;
		}
		break;
	case CMD_ADDR_SCRIPT:
		color_r = 0;
   		color_g = 0;
   		color_b = 0;
		script_id = input_buffer[1];
		break;
	default:
		//return(build_reply(output_buffer_length, output_buffer, 0, 0));
		break;
	}

	//return(build_reply(output_buffer_length, output_buffer, 0, 0));
}


void timer2_Init(void)
{
	uint8_t sreg = SREG;
	cli();
	TCCR2B = CLK8;
	TIMSK2 |= (1 << TOIE2);
	SREG = sreg;
	sei();
}


void soft_PWM_Pulse(void)
{
	if (++softPWM_Pulse_Counter == 0) {
		if (color_r > 0) {
			PWM_LED_R_ON;
		}
		if (color_g > 0) {
			PWM_LED_G_ON;
		}
		if (color_b > 0) {
			PWM_LED_B_ON;
		}
	}

	if (color_r != 255 && softPWM_Pulse_Counter == color_r) {
		PWM_LED_R_OFF;
	}
	if (color_g != 255 && softPWM_Pulse_Counter == color_g) {
		PWM_LED_G_OFF;
	}
	if (color_b != 255 && softPWM_Pulse_Counter == color_b) {
		PWM_LED_B_OFF;
	}
}

int main (void)
{
	replybuffer[0] = 0;
	replybuffer[1] = 0;
	color_r = 0;
	color_g = 0;
	color_b = 0;

	ws2801_init();

	STATUS_LED_INIT;
	STATUS_LED_ON;

	PWM_LED_INIT;
	PWM_LED_R_OFF;
	PWM_LED_G_OFF;
	PWM_LED_B_OFF;

	timer0_Init();
	timer2_Init();

	start_twi_slave(SLAVE_ADDRESS, 0, twi_callback, idle_callback);

	while(1) {
		// yip yip yip yip nok nok nok ;)
	}

	return 0; // we never reach this
}


ISR(TIMER2_OVF_vect)
{
	soft_PWM_Pulse();
}
