#ifndef _BOARD_HM_LC_Sw1_PI_H
#define _BOARD_HM_LC_Sw1_PI_H

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

#define CC1100_GDO0_DDR         DDRD
#define CC1100_GDO0_PORT        PORTD
#define CC1100_GDO0_IN          PIND
#define CC1100_GDO0_PIN         3
#define CC1100_GDO2_DDR         DDRD
#define CC1100_GDO2_PORT        PORTD
#define CC1100_GDO2_IN          PIND
#define CC1100_GDO2_PIN         2
#define CC1100_GDO2_INT         INT0
#define CC1100_GDO2_INTVECT     INT0_vect
#define CC1100_GDO2_ISC         ISC00
#define CC1100_GDO2_EICR        EICRA

#define RESEND_AFTER 		10000

#define BOOT_RADIO          	
#define RADIO_CHANNEL           0x2904
#define RADIO_ALWAYS

/* where is the conditional switch located? */
#define BOOT_PORT          	PORTA
#define BOOT_DDR           	DDRA
#define BOOT_IN           	PINA
#define BOOT_PIN          	0
#define BOOT_LOWACTIVE          	

#define LED_PORT		PORTD
#define LED_DDR			DDRD
#define LED_PIN			4

#define REL_DDR                 DDRB
#define REL_PORT                PORTB
#define REL_PIN                 0

#endif

