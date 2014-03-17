#ifndef _BOARD_COC_H
#define _BOARD_COC_H

/*********************************/
/* Will I listen on serial line? */
/*********************************/

#define BOOT_SERIAL          	

/* UART Baudrate */
#define BAUDRATE 38400
/* use "Double Speed Operation" */
#define UART_DOUBLESPEED

#define UART_STACKED

/* where is the conditional switch located? */

#define BOOT_PORT               PORTB
#define BOOT_DDR                DDRB
#define BOOT_IN                 PINB
#define BOOT_PIN                3
#define BOOT_LOWACTIVE

#define SWITCH_PORT             PORTB
#define SWITCH_DDR              DDRB
#define SWITCH_IN               PINB
#define SWITCH_PIN              0
#define SWITCH_LOWACTIVE

#define LED_PORT                PORTC
#define LED_DDR                 DDRC
#define LED_PIN                 6

#endif

