#include <avr/io.h>

#include "leds.h"

static uint8_t oleds;

static void leds_set( uint8_t v ) {
     
     if (oleds == v)
	  return;

     // swap bits
     uint8_t o = 0;
     if (v & 1) o |= 8;
     if (v & 2) o |= 4;
     if (v & 4) o |= 2;
     if (v & 8) o |= 1;

     PORTA &= ~0x3c;
     PORTA |= ((o<<2) & 0x3c);
     
     leds_value = v;
     oleds      = v;
}

void leds_init( void ) {
     DDRA |= 0x3c;

     oleds = 1;
     leds_set( 0x0f ); // all LED on
}

void leds_periodic( void ) {

     leds_set( leds_value );

}


