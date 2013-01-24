
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "leds.h"
#include "clock.h"
#include "serial.h"
#include "rtc.h"
#include "s0.h"

uint32_t last_seconds = 0;
uint32_t tick = 0;

int main(void) {

     wdt_enable(WDTO_1S);

     leds_init();
     clock_init();
     serial_init();
     rtc_init();
     s0_init();

     sei();

     while (1) { 
       wdt_reset();

       serial_periodic();
       s0_periodic();

     }

}
