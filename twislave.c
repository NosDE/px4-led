#include <util/twi.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "twislave.h"


enum {
	ss_state_idle,
	ss_state_receiving,
	ss_state_new_data,
} startstop_state_t;


static void (*idle_callback)(void);
static void (*data_callback)(uint8_t input_buffer_length, const uint8_t *input_buffer, uint8_t *output_buffer_length, uint8_t *output_buffer);


static uint8_t	ss_state;
static uint8_t	input_buffer[HW_TWI_BUFFER_SIZE];
static uint8_t	input_buffer_length;
static uint8_t	output_buffer[HW_TWI_BUFFER_SIZE];
static uint8_t	output_buffer_length;
static uint8_t	output_buffer_current;
static uint8_t	slave_address;


void init_twi_slave(uint8_t adr)
{
	TWAR= adr; // set own twi address
	TWCR &= ~(1<<TWSTA)|(1<<TWSTO);
	TWCR|= (1<<TWEA) | (1<<TWEN)|(1<<TWIE); 	
	sei();
}


void start_twi_slave(uint8_t slave_address_in, uint8_t use_sleep,
			void (*data_callback_in)(uint8_t input_buffer_length, const uint8_t *input_buffer,
			uint8_t *output_buffer_length, uint8_t *output_buffer),
			void (*idle_callback_in)(void))
{
	uint8_t	call_datacallback = 0;

	slave_address = slave_address_in;
	data_callback = data_callback_in;
	idle_callback = idle_callback_in;

	input_buffer_length	= 0;
	output_buffer_length	= 0;
	output_buffer_current	= 0;
	
	ss_state = ss_state_idle;

	init_twi_slave(slave_address_in);

	for (;;) {
		if(idle_callback) {
			idle_callback();
		}

		if (ss_state == ss_state_new_data) {
			call_datacallback = 1;
		}

		if(call_datacallback) {
			output_buffer_length	= 0;
			output_buffer_current	= 0;
			
			data_callback(input_buffer_length, input_buffer, &output_buffer_length, output_buffer);
			
			input_buffer_length		= 0;
			call_datacallback		= 0;
			ss_state = ss_state_idle;
		}
	}
}

#define TWCR_ACK TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);  
#define TWCR_NACK TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);
#define TWCR_RESET TWCR = (1<<TWEN)|(1<<TWIE)|(1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC);  

ISR (TWI_vect)  
{
	uint8_t data=0;

	switch (TW_STATUS) {
	// slave seceiver 
	case TW_SR_SLA_ACK:	// 0xaa slave seceiver
		TWCR_ACK;	// next byte, then ack
		break;
	case TW_SR_DATA_ACK:	// 0xaa slave receiver, get one byte
		data=TWDR;	// readout data
		if(input_buffer_length < (HW_TWI_BUFFER_SIZE - 1)) {
			input_buffer[input_buffer_length++] = data;
		}
		TWCR_ACK;
		break;

	// slave transmitter
	case TW_ST_SLA_ACK:	// 0xab slave read mode, send ack
	// no break here. execute following code
	case TW_ST_DATA_ACK: 	// 0xab slave sransmitter, data requested
		if(output_buffer_current < output_buffer_length) {
			TWDR = output_buffer[output_buffer_current++];
		} else {
			TWDR = 0x00;	// no more data, but cannot send "nothing" or "nak"
		}
		TWCR_ACK;
		break;
	case TW_SR_STOP:
		TWCR_ACK;
		ss_state = ss_state_new_data;
		break;
	case TW_SR_DATA_NACK: // 0x88 
	case TW_ST_LAST_DATA: // 0xC8  last data byte in twdr has been transmitted (twea = "0"); ack has been received
	case TW_ST_DATA_NACK: // 0xC0 no more data requested
	default: 	
		TWCR_RESET;
		break;
	
	}
}
