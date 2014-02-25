#ifndef _TWISLAVE_H
#define _TWISLAVE_H

#include <util/twi.h>
#include <avr/interrupt.h>
#include <stdint.h>

enum {
	HW_TWI_BUFFER_SIZE = 16
};


/************************* no changes below this ********************************/

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
	#error "This library requires AVR-GCC 3.4.5 or later, update to newer AVR-GCC compiler !"
#endif


void init_twi_slave(uint8_t adr);
void start_twi_slave(
	uint8_t slave_address, uint8_t use_sleep, 
	void (*data_callback)(uint8_t input_buffer_length, const uint8_t *input_buffer, uint8_t *output_buffer_length, uint8_t *output_buffer),
	void (*idle_callback)(void)
);

#endif // _TWISLAVE_H
