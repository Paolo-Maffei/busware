#ifndef SERIAL_H
#define SERIAL_H

#define UART_BAUD_RATE 38400

void serial_init( void );
void serial_periodic( void );
void txfer_radio_msg( char *msg );

#endif
