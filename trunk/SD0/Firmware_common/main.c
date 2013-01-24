
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "serial.h"
#include "leds.h"
#include "clock.h"
#include "s0.h"
#include "rtc.h"
#include "adc.h"
#include "radio.h"
#include "uart.h"

uint32_t last_seconds = 0;

int main(void) {

     wdt_enable(WDTO_1S);

     leds_init();

     clock_init();

     serial_init();

     rtc_init();

//     adc_init();

     s0_init();

//     radio_init();

     sei();

     uint8_t i = 0;

     PORTA |= _BV( 6 );  // Pull BL
     
     while (1) {
	  wdt_reset();

/*

	  // BootSel? bridge to radio natively!
	  if (!bit_is_set( PINA, 6 )){

	       // Reset Radio into BL mode
	       radio_bootloader();

	       while (!bit_is_set( PINA, 6 )) {
		    
		    wdt_reset();

		    uint16_t c = uart_getc();
		    if ( (c & 0xff00) == 0 )
			 uart1_putc( (unsigned char)c );
	       
		    c = uart1_getc();
		    if ( (c & 0xff00) == 0 )
			 uart_putc( (unsigned char)c );
	       }
	       
	       radio_reset();

	       continue;
	  }
*/
	  
	  serial_periodic();
	  leds_periodic();
//	  radio_periodic();

	  // every second ...
	  if (now_sec() == last_seconds)
	       continue;
	  
	  last_seconds = now_sec();

	  leds_value = 0;

	  if (last_seconds % 10 == 0) {
	       leds_value = 0xf;
	       s0_periodic();
	  }
	

/*
	  if (last_seconds % 30 == 0) {

	       // check channels and indicate error ...
	       for (i=0;i<4;i++)
		    if (adc_get_ch(i)<5) {
			 leds_value |= _BV( i );
			 leds_value |= _BV( 6 );
		    }

	  }
*/
	  
  
     }

}
