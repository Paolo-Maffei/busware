
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "serial.h"

static char buffer[256];
static uint8_t buf_cnt;

int uart_putchar (char data, FILE * stream) {
     uart_putc( data );
     return 0;
}

static void parse_buffer( void ) {

     switch( buffer[0] ) {
	  
     case '.':
	  printf_P( PSTR("%s\r\n"), buffer );
	  break;

     case 'V':
     case 'v':
	  printf_P( PSTR("V %d.%d\r\n"), SWVERSIONMAJOR, SWVERSIONMINOR );
	  break;

     case 'L':
     case 'l':
	  printf_P( PSTR("L %02X\r\n"), PINB );
	  break;

     case 'X':
     case 'x':

	  printf_P( PSTR("rebooting\r\n") );

	  while (1);
	  
	  break;
	  
	  
     default:
        printf_P( PSTR("? %02X\r\n"), buffer[0] );

     }
	  
}

void serial_init( void ) {
     uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) ); 
     
     fdevopen(&uart_putchar,NULL);

//     printf_P( PSTR("S0 V%d.%d up\r\n"),  SWVERSIONMAJOR, SWVERSIONMINOR );

     buffer[0] = 0;
     buf_cnt   = 0;
}

void serial_periodic( void ) {

     // transfer uart buffer
     while (1) {
	  
	  uint16_t c = uart_getc();

	  if (c == UART_NO_DATA)
	       break;
	  
	  if (c & 0xff00)
	       continue;
	  
	  if (c == 10)
	       continue;
	  
          // filter RPi rubbish
	  if (c > 127 )
	       continue;
	  
	  buffer[buf_cnt++] = c;
	  buffer[buf_cnt]   = 0;
     }
     
          
     // EOT?
     if (buf_cnt && (buffer[buf_cnt-1] == 13)) {
	  
	  buffer[--buf_cnt] = 0;
	  
	  parse_buffer();
	  
	  buffer[0] = 0;
	  buf_cnt   = 0;
     }
     
}

