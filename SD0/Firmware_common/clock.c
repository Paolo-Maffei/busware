#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "clock.h"
#include "leds.h"

static volatile uint16_t ms_ticks;
static volatile uint32_t sec_ticks;

ISR(TIMER1_COMPA_vect) {

     if (++ms_ticks>999) {
	  sec_ticks++;
	  ms_ticks = 0;

	  // TuxRail watchdog
	  if (wd_set) {

	       if (wd_ticks++>wd_set) {
/*		    
		    // reset TuxRail
		    DDRB  |= _BV( PB2 );
		    PORTB |= _BV( PB2 );

		    _delay_ms(5);

		    // we should been dead until now ...

		    PORTB &= ~_BV( PB2 );
		    DDRB  &= ~_BV( PB2 );

		    while (1); // wait for reset
*/

	       }

	  }

     }
     
     TIFR1 |= _BV( OCF1A );
}

void clock_init( void ) {

     // Timer1 @ 1MHz == 1us
     TCNT1  = 0;
     TCCR1A = 0;
     TCCR1B = _BV(WGM12) | _BV(CS11);   // CTC & prescale 8 - 1usec
     OCR1A  = 1000; // equals 1 ms
     TIMSK1 = _BV( OCIE1A );

     ms_ticks  = 0;
     sec_ticks = 0;

     wd_set   = 0;
     wd_ticks = 0;
     
}

uint32_t ms_now( void ) {
     return ms_ticks + ( 1000 * sec_ticks );
}

uint32_t now_sec( void ) {
     return sec_ticks;
}

uint16_t now_msec( void ) {
     return ms_ticks;
}
