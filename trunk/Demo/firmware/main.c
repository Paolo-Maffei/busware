#include <avr/io.h>
#include <util/delay.h>


void hallo(void) {
	
}

int main(void) {
    DDRB = _BV( PB1 );           /* make the LED pin an output */
	for(;;) {
		char i;
        for(i = 0; i < 10; i++){
            _delay_ms(50);  /* max is 262.14 ms / F_CPU in MHz */
        }
        PORTB ^= _BV( PB1 );    /* toggle the LED */

		hallo();

    }
	return 0;               /* never reached */
}
