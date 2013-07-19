#ifndef _BOARD_CCD_H
#define _BOARD_CCD_H

/*********************************/
/* Will I listen on serial line? */
/*********************************/

#define BOOT_SERIAL          	

/* UART Baudrate */
#define BAUDRATE 38400
/* use "Double Speed Operation" */
#define UART_DOUBLESPEED
/* use second UART on mega128 / can128 / mega162 / mega324p / mega644p */
//#define UART_USE_SECOND

/* where is the conditional switch located? */

#define BOOT_PORT               PORTB
#define BOOT_DDR                DDRB
#define BOOT_IN                 PINB
#define BOOT_PIN                0
#define BOOT_LOWACTIVE

#define LED_PORT                PORTC
#define LED_DDR                 DDRC
#define LED_PIN                 6

#endif

