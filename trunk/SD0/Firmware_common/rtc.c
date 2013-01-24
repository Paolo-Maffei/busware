#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "i2cmaster.h"
#include "rtc.h"

#define DevRTC 0xd0
#define RTC_RAM_START 0x18

uint8_t rtc_read(unsigned char addr) {
     uint8_t ret;
     i2c_start_wait(DevRTC);                  // set device address and write mode
     i2c_write(addr);                         // write address = 0
     i2c_rep_start(DevRTC+1);                 // set device address and read mode
     ret = i2c_readNak();                     // read one byte form address 0
     i2c_stop();                              // set stop condition = release bus
     return ret;
}

void rtc_write(unsigned char addr, uint8_t data) {
     i2c_start_wait(DevRTC);                  // set device address and write mode
     i2c_write(addr);                         // write address
     uint8_t ret = i2c_write(data);           // ret=0 -> Ok, ret=1 -> no ACK 
     i2c_stop();                              // set stop conditon = release bus
}

void rtc_init(void) {
     i2c_init();                              // init I2C interface

/*

     // set yy / mm / dd
     rtc_write( 6, 0x10 );
     rtc_write( 5, 0x08 );
     rtc_write( 4, 0x28 );

     // set sec / min / hour
     rtc_write( 0, 0 );
     rtc_write( 1, 0x56 );
     rtc_write( 2, 0x02 );

*/

}

void rtc_write_ticks( uint8_t ch, uint32_t val ) {
     rtc_write( RTC_RAM_START + (ch*4) + 0, val & 0xff );
     rtc_write( RTC_RAM_START + (ch*4) + 1, (val>>8) & 0xff );
     rtc_write( RTC_RAM_START + (ch*4) + 2, (val>>16) & 0xff );
     rtc_write( RTC_RAM_START + (ch*4) + 3, (val>>24) & 0xff );
}
	       
uint32_t rtc_read_ticks( uint8_t ch ) {

     uint32_t val = rtc_read( RTC_RAM_START + (ch*4) + 0 );
     val |= ((uint32_t)rtc_read( RTC_RAM_START + (ch*4) + 1 ) << 8);
     val |= ((uint32_t)rtc_read( RTC_RAM_START + (ch*4) + 2 ) << 16);
     val |= ((uint32_t)rtc_read( RTC_RAM_START + (ch*4) + 3 ) << 24);

     return val;
}

void rtc_read_clock( rtc_clock_t *datetime ) {

     datetime->sec   = rtc_read( 0 );
     datetime->min   = rtc_read( 1 );
     datetime->hour  = rtc_read( 2 );
     datetime->day   = rtc_read( 3 );
     datetime->date  = rtc_read( 4 );
     datetime->month = rtc_read( 5 );
     datetime->year  = rtc_read( 6 );
     
}

void rtc_write_clock( rtc_clock_t *datetime ) {

     rtc_write( 0, datetime->sec );
     rtc_write( 1, datetime->min );
     rtc_write( 2, datetime->hour );
     rtc_write( 3, datetime->day );
     rtc_write( 4, datetime->date );
     rtc_write( 5, datetime->month );
     rtc_write( 6, datetime->year );
     
}


