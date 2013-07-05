#ifndef _BOARD_CUNO2_H
#define _BOARD_CUNO2_H

#define SPI_PORT                PORTB
#define SPI_DDR                 DDRB
#define SPI_IN                  PINB
#define SPI_SS                  4
#define SPI_MISO                6
#define SPI_MOSI                5
#define SPI_SCLK                7

#define CC1100_CS_DDR         	SPI_DDR
#define CC1100_CS_PORT        	SPI_PORT
#define CC1100_CS_PIN         	SPI_SS

#define CC1100_GDO0_DDR         DDRB
#define CC1100_GDO0_PORT        PORTB
#define CC1100_GDO0_IN          PINB 
#define CC1100_GDO0_PIN         1
#define CC1100_GDO2_DDR         DDRB
#define CC1100_GDO2_PORT        PORTB
#define CC1100_GDO2_IN          PINB
#define CC1100_GDO2_PIN         2
#define CC1100_GDO2_INT         INT2
#define CC1100_GDO2_INTVECT     INT2_vect
#define CC1100_GDO2_ISC         ISC20
#define CC1100_GDO2_EICR        EICRA

#define RESEND_AFTER 		10000

/*********************************/
/* Will I listen on serial line? */
/*********************************/

#define BOOT_RADIO          	
#define RADIO_CHANNEL           0x2904
//#define RADIO_ALWAYS

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

#define BOOT_PORT               PORTA
#define BOOT_DDR                DDRA
#define BOOT_IN                 PINA
#define BOOT_PIN                1
#define BOOT_LOWACTIVE

#define BOOT_RADIO_PORT        	BOOT_PORT
#define BOOT_RADIO_DDR         	BOOT_DDR
#define BOOT_RADIO_IN          	BOOT_IN
#define BOOT_RADIO_PIN         	BOOT_PIN
#define BOOT_RADIO_LOWACTIVE

#define LED_PORT                PORTB
#define LED_DDR                 DDRB
#define LED_PIN                 0

#endif

