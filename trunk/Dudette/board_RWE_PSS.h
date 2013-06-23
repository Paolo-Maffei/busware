#ifndef _BOARD_RWE_PSS_H
#define _BOARD_RWE_PSS_H

#define SPI_PORT                PORTB
#define SPI_DDR                 DDRB
#define SPI_IN                  PINB
#define SPI_SS                  2
#define SPI_MISO                4
#define SPI_MOSI                3
#define SPI_SCLK                5

#define CC1100_CS_DDR         	SPI_DDR
#define CC1100_CS_PORT        	SPI_PORT
#define CC1100_CS_PIN         	SPI_SS

#define CC1100_GDO0_DDR         DDRD
#define CC1100_GDO0_PORT        PORTD
#define CC1100_GDO0_PIN         2
#define CC1100_GDO2_DDR         DDRD
#define CC1100_GDO2_PORT        PORTD
#define CC1100_GDO2_IN          PIND
#define CC1100_GDO2_PIN         3
#define CC1100_GDO2_INT         INT1
#define CC1100_GDO2_INTVECT     INT1_vect
#define CC1100_GDO2_ISC         ISC10
#define CC1100_GDO2_EICR        EICRA

#define RESEND_AFTER 		10000

#define BOOT_RADIO          	
#define RADIO_CHANNEL           0x2904
//#define RADIO_ALWAYS

/* where is the conditional switch located? */
#define BOOT_PORT          	PORTD
#define BOOT_DDR           	DDRD
#define BOOT_IN           	PIND
#define BOOT_PIN          	0
#define BOOT_LOWACTIVE          	

#define LED_PORT		PORTB
#define LED_DDR			DDRB
#define LED_PIN			0

#endif

