#ifndef _BOARD_CSM_H
#define _BOARD_CSM_H

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
#define CC1100_GDO2_DDR         DDRD
#define CC1100_GDO2_PORT        PORTD
#define CC1100_GDO2_IN          PIND
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
#define RADIO_ALWAYS

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
#ifdef CSM
#define BOOT_PORT          	PORTD
#define BOOT_DDR           	DDRD
#define BOOT_IN           	PIND
#define BOOT_PIN          	3
#define BOOT_LOWACTIVE          	
#else
#define BOOT_PORT          	PORTD
#define BOOT_DDR           	DDRD
#define BOOT_IN           	PIND
#define BOOT_PIN          	5
#endif

#define LED_PORT		PORTA
#define LED_DDR			DDRA
#define LED_PIN			1

#define ALWAYSLED_PORT		PORTA
#define ALWAYSLED_DDR		DDRA
#define ALWAYSLED_PIN		0

#endif

