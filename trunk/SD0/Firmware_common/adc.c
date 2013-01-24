#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "adc.h"

uint16_t adc_get_ch( uint8_t idx ) {
     uint8_t i;
     uint16_t result = 0;
     
     ADMUX = _BV(REFS1) | (idx + 4); // 1.1V Ref
     
     // Frequenzvorteiler setzen auf 64 (1) und ADC aktivieren (1)
     ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS2); 
     
     ADCSRA |= (1<<ADSC);              // Dummy readout
     while ( ADCSRA & (1<<ADSC) );     // wait for converter to finish
     
     /* Eigentliche Messung - Mittelwert aus 2 aufeinanderfolgenden Wandlungen */
     for(i=0; i<2; i++) {
	  ADCSRA |= (1<<ADSC);            // eine Wandlung "single conversion"
	  while ( ADCSRA & (1<<ADSC) );   // auf Abschluss der Konvertierung warten
	  result += ADCW;                 // Wandlungsergebnisse aufaddieren
     }

     ADCSRA &= ~(1<<ADEN);                // ADC deaktivieren (2)
     
     result /= 2; 
     
     return result;
}


void adc_init( void ) {

     // Inputs - not pulled!
     DDRA  = 0;
     PORTA = 0;

}
