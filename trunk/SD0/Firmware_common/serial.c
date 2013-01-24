
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "serial.h"
#include "s0.h"
#include "clock.h"
#include "rtc.h"
#include "adc.h"

static char buffer[256];
static uint8_t buf_cnt;

int uart_putchar (char data, FILE * stream) {
     uart_putc( data );
     return 0;
}

static void parse_buffer( void ) {
     Channel_t ch_data[4];
     uint8_t  ch  = 0;
     uint8_t  res = 0;
     uint32_t val = 0;
     rtc_clock_t datetime;

     int year, month, date, hour, min, sec;


     switch( buffer[0] ) {
	  
     case '.':
	  printf_P( PSTR("%s\r\n"), buffer );
	  break;

     case '*':
	  uart1_puts( buffer+1 );
	  uart1_putc( 13 );
	  break;

     case 'V':
     case 'v':
	  printf_P( PSTR("%d.%d\r\n"), SWVERSIONMAJOR, SWVERSIONMINOR );
	  break;

     case 'L':
     case 'l':
	  printf_P( PSTR("%02X\r\n"), PINA );
	  break;

     case 'A':
     case 'a':

	  printf_P( PSTR("%i, %i, %i, %i\r\n"), 
		    adc_get_ch(0),
		    adc_get_ch(1),
		    adc_get_ch(2),
		    adc_get_ch(3)
	       );
	  
	  break;

     case 'c':


	  res = sscanf_P( &buffer[1], PSTR ("20%2x-%2x-%2x %2x:%2x:%2x"), &year, &month, &date, &hour, &min, &sec);

	  // set clock?
	  if (res==6) {
/*
	       printf_P( PSTR(" setting to: 20%02X-%02X-%02X %02X:%02X:%02X\r\n"), 
			 year,
			 month,
			 date,
			 hour,
			 min,
			 sec
		    );
*/

	       datetime.year  = year;
	       datetime.month = month;
	       datetime.date  = date;
	       datetime.hour  = hour;
	       datetime.min   = min;
	       datetime.sec   = sec;
	  
	       rtc_write_clock( &datetime );

	  }
	  
	  rtc_read_clock( &datetime );
	  
	  printf_P( PSTR("20%02X-%02X-%02X %02X:%02X:%02X\r\n"), 
		    datetime.year,
		    datetime.month,
		    datetime.date,
		    datetime.hour,
		    datetime.min,
		    datetime.sec
	       );


	  break;

     case 'C':

	  val = ms_now();
	  
	  printf_P( PSTR("%lu msec\r\n"), val );
	  
	  break;

	  
     case 'T':
     case 't':

	  s0_getChannelData( 0, &ch_data[0], 0 );
	  s0_getChannelData( 1, &ch_data[1], 0 );
	  s0_getChannelData( 2, &ch_data[2], 0 );
	  s0_getChannelData( 3, &ch_data[3], 0 );
	  
	  printf_P( PSTR("%lu, %lu, %lu, %lu\r\n"), 
		    ch_data[0].count, 
		    ch_data[1].count, 
		    ch_data[2].count, 
		    ch_data[3].count 
	       );
	  
	  break;

     case 'P':
     case 'p':

	  s0_getChannelData( 0, &ch_data[0], 0 );
	  s0_getChannelData( 1, &ch_data[1], 0 );
	  s0_getChannelData( 2, &ch_data[2], 0 );
	  s0_getChannelData( 3, &ch_data[3], 0 );

	  printf_P( PSTR("%lu, %lu, %lu, %lu\r\n"), 
		    ch_data[0].period,
		    ch_data[1].period, 
		    ch_data[2].period,
		    ch_data[3].period
	       );
	  
	  break;

     case 'r':
     case 'R':

	  switch( buffer[1] ) {
	       
	  case '1':
	  case 'B':
	  case 'b':
	       ch = 1;
	       break;
	       
	  case '2':
	  case 'C':
	  case 'c':
	       ch = 2;
	       break;
	       
	  case '3':
	  case 'D':
	  case 'd':
	       ch = 3;
	       break;
	       
	  default:
	       ch = 0;
	       
	  }

	  s0_getChannelData( ch, &ch_data[0], (buffer[0] == 'R') );
	  
	  printf_P( PSTR("%lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\r\n"), 
		     ms_now(),
		     ch_data[0].count,   // 1
		     ch_data[0].pcount,  // 2
		     ch_data[0].period,  // 3
		     ch_data[0].avg_cnt, // 4
		     ch_data[0].avg_sum, // 5
		     ch_data[0].min,     // 6
		     ch_data[0].max,     // 7
		     ch_data[0].last     // 8
	       );
	  
	  break;

     case 's':
     case 'S':

	  res = sscanf_P( &buffer[1], PSTR ("%i %lu"), &ch, &val);

	  if (ch<4) {

	       if (res == 2)
		    s0_setChannelTicks( ch, val );
	       
	  } else 
	       ch = 0;
	  

	  s0_getChannelData( ch, &ch_data[0], 0 );
	  printf_P( PSTR("%lu\r\n"), ch_data[0].count );
	  
	  break;

     case 'o':
     case 'O':

	  res = sscanf_P( &buffer[1], PSTR ("%i"), &ch);

	  if (res == 111) { // disabled ...
	       printf_P( PSTR("OK %d\r\n"), ch ? 1 : 0 );
	  } else {
	       printf_P( PSTR("?\r\n") );
	  }
	       
	  break;
	  
     case 'w':
     case 'W':

	  res = sscanf_P( &buffer[1], PSTR ("%i"), &ch);

	  if (res == 1) { // disabled ...
	       wd_set = ch;
	  }

	  printf_P( PSTR("OK %d\r\n"), wd_set );
	       
	  break;
	  

     case 'X':
     case 'x':

	  printf_P( PSTR("rebooting\r\n") );

	  while (1);
	  
	  break;
	  
	  
     default:
        printf_P( PSTR("?\r\n") );

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
	  
	  buffer[buf_cnt++] = c;
	  buffer[buf_cnt]   = 0;
     }
     
          
     // EOT?
     if (buf_cnt && (buffer[buf_cnt-1] == 13)) {
	  
	  wd_ticks = 0;

	  buffer[--buf_cnt] = 0;
	  
	  parse_buffer();
	  
	  buffer[0] = 0;
	  buf_cnt   = 0;
     }
     
}

void txfer_radio_msg( char *msg ) {
     printf_P( PSTR("*%s\r\n"), msg );
}

