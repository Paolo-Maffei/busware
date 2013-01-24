#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "clock.h"
#include "leds.h"
#include "s0.h"
#include "rtc.h"

static volatile uint8_t port_bits;
static volatile Channel_t CH[4];

static void impuls_action( uint8_t ch ) {
     uint32_t now = ms_now();
     
     if (ch>3)
	  return;

     CH[ch].count++;
     CH[ch].pcount++;

     if (CH[ch].last) {
	  CH[ch].period = now - CH[ch].last;
	  
	  CH[ch].avg_cnt++;
	  CH[ch].avg_sum += CH[ch].period;

	  if (CH[ch].period<=CH[ch].min)
	       CH[ch].min = CH[ch].period;
	  if (CH[ch].period>=CH[ch].max)
	       CH[ch].max = CH[ch].period;
     }
     
     CH[ch].last = now;
     
     leds_value |= _BV( ch );
}


ISR(PCINT1_vect) {
     uint8_t in   = (PINB & 0x1e) >> 1;
     uint8_t diff = (in ^ port_bits) & port_bits;
     
     if ( diff & 2 )
	  impuls_action( 0 );
     
     if ( diff & 1 )
	  impuls_action( 1 );
     
     if ( diff & 4 )
	  impuls_action( 2 );
     
     if ( diff & 8 )
	  impuls_action( 3 );
     
     port_bits = in;
}

void s0_init( void ) {

     port_bits = 0xff;

     for (uint8_t i=0;i<4;i++) {
	  CH[i].count   = rtc_read_ticks(i);
	  CH[i].pcount  = 0;
	  CH[i].period  = 0;
	  CH[i].min     = -1;
	  CH[i].max     = 0;
	  CH[i].avg_cnt = 0;
	  CH[i].avg_sum = 0;
	  CH[i].adc     = 1;
     }

     // Inputs - not pulled!
     DDRB  = 0;
     PORTB = 0;

     PCMSK1 = 0x1e;
     PCICR |= _BV( PCIE1 );
     
}

uint8_t s0_getChannelData( uint8_t ch, Channel_t *data, uint8_t reset ) {

     if (ch>3)
	  return 0;

     if (!data)
	  return 0;

     // ATOMIC
     AVR_ENTER_CRITICAL_REGION();

     data->count  = CH[ch].count;
     data->pcount = CH[ch].pcount;
     data->period = CH[ch].period;
     data->min    = CH[ch].min;
     data->max    = CH[ch].max;
     data->avg_cnt= CH[ch].avg_cnt;
     data->avg_sum= CH[ch].avg_sum;
     data->last   = CH[ch].last;
     data->adc    = CH[ch].adc;
     
     // Reset statistic data
     if (reset) {

	  // Clear the "last periode" if during two "Resets" no ticks have been counted - indicates: No Power consumtion
	  if (!CH[ch].pcount)
	       CH[ch].period = 0;
	  
	  CH[ch].avg_cnt = 0;
	  CH[ch].avg_sum = 0;
	  CH[ch].min = -1;
	  CH[ch].max = 0;
	  CH[ch].pcount = 0;
     }

     AVR_LEAVE_CRITICAL_REGION();
     // END ATOMIC


     return 1;
}

uint8_t s0_setChannelTicks( uint8_t ch, uint32_t ticks ) {

     if (ch>3)
	  return 0;
     
     // ATOMIC
     AVR_ENTER_CRITICAL_REGION();
     
     CH[ch].count = ticks;
     
     AVR_LEAVE_CRITICAL_REGION();
     // END ATOMIC

     return 1;
}


void s0_periodic( void ) {

     for (uint8_t i=0;i<4;i++)
	  rtc_write_ticks( i, CH[i].count );
     
}

