#ifndef S0_H
#define S0_H

#include <avr/io.h>

typedef struct  {
     uint32_t  count; 
     uint32_t  avg_cnt; 
     uint32_t  avg_sum; 
     uint32_t  min; 
     uint32_t  max; 
     uint32_t  period; 
     uint32_t  pcount; 
     uint32_t  last; 
     uint32_t  adc; 
} Channel_t;

void s0_init( void );
void s0_periodic( void );
uint8_t s0_getChannelData( uint8_t ch, Channel_t *data, uint8_t reset );
uint8_t s0_setChannelTicks( uint8_t ch, uint32_t ticks );

#define AVR_ENTER_CRITICAL_REGION() uint8_t sreg = SREG; cli()
#define AVR_LEAVE_CRITICAL_REGION() SREG = sreg

#endif
