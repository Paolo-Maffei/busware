#ifndef _BOARD_HM_OU_LED16_H
#define _BOARD_HM_OU_LED16_H

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
#define CC1100_GDO0_IN          PIND
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
/* this is: send / reset button */

#define BOOT_PORT          	PORTC
#define BOOT_DDR           	DDRC
#define BOOT_IN           	PINC
#define BOOT_PIN          	1
#define BOOT_LOWACTIVE          	

#define BOOT_RADIO_PORT         BOOT_PORT
#define BOOT_RADIO_DDR          BOOT_DDR
#define BOOT_RADIO_IN           BOOT_IN
#define BOOT_RADIO_PIN          BOOT_PIN
#define BOOT_RADIO_LOWACTIVE

#endif

