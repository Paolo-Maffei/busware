#ifndef RTC_H
#define RTC_H

struct rtc_clock {
     uint8_t sec;
     uint8_t min;
     uint8_t hour;
     uint8_t day;
     uint8_t date;
     uint8_t month;
     uint8_t year;
} __attribute__((__packed__));

typedef struct rtc_clock rtc_clock_t;

void rtc_init( void );

unsigned char rtc_read(unsigned char addr);
void rtc_write(unsigned char addr, unsigned char data);

void rtc_write_ticks( uint8_t ch, uint32_t val );
uint32_t rtc_read_ticks( uint8_t ch );

void rtc_read_clock( rtc_clock_t *datetime );
void rtc_write_clock( rtc_clock_t *datetime );

#endif
