#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "radio.h"
#include "serial.h"

static char buffer[256];
static uint8_t buf_cnt;

void radio_reset( void ) {
}

void radio_bootloader( void ) {
}


void radio_init( void ) {

     radio_reset();

     uart1_init( UART_BAUD_SELECT_DOUBLE_SPEED(38400,F_CPU) ); 
     
     buffer[0] = 0;
     buf_cnt   = 0;
}

void radio_periodic( void ) {

     // transfer uart buffer
     while (1) {
	  
	  uint16_t c = uart1_getc();
	  
	  if (c == UART_NO_DATA)
	       break;
	  
	  if (c & 0xff00)
	       continue;
	  
	  if (c == 10)
	       continue;
	  
	  buffer[buf_cnt++] = c;
	  buffer[buf_cnt]   = 0;
     }
     
          
     // EOT?
     if (buf_cnt && (buffer[buf_cnt-1] == 13)) {
	  
	  buffer[--buf_cnt] = 0;

	  txfer_radio_msg( buffer );
	  
	  buffer[0] = 0;
	  buf_cnt   = 0;
     }
     
}
