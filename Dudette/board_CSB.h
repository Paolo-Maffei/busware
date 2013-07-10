#ifndef _BOARD_CSB_H
#define _BOARD_CSB_H

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
#define CC1100_GDO0_PIN         2
#define CC1100_GDO2_DDR         DDRD
#define CC1100_GDO2_PORT        PORTD
#define CC1100_GDO2_IN          PIND
#define CC1100_GDO2_PIN         2
#define CC1100_GDO2_INT         INT0
#define CC1100_GDO2_INTVECT     INT0_vect
#define CC1100_GDO2_ISC         ISC00
#define CC1100_GDO2_EICR        EICRA

#define RESEND_AFTER 		10000

/*********************************/
/* Will I listen on serial line? */
/*********************************/

#define BOOT_RADIO          	
#define RADIO_CHANNEL           0x2904
#define RADIO_ALWAYS

#define LED_PORT                PORTB
#define LED_DDR                 DDRB
#define LED_PIN                 0

#endif

