#ifndef CLOCK_H
#define CLOCK_H

void clock_init( void );
uint32_t ms_now( void );
uint32_t now_sec( void );
uint16_t now_msec( void );

volatile uint16_t wd_ticks;
uint16_t wd_set;

#endif
