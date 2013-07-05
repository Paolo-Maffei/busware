#ifndef _BOARD_H
#define _BOARD_H

#define SPI_PORT                PORTB
#define SPI_DDR                 DDRB
#define SPI_IN                  PINB
#define SPI_SS                  0
#define SPI_MISO                3
#define SPI_MOSI                2
#define SPI_SCLK                1

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
#define CC1100_GDO2_INT         INT2
#define CC1100_GDO2_INTVECT     INT2_vect
#define CC1100_GDO2_ISC         ISC20
#define CC1100_GDO2_EICR        EICRA

#define RESEND_AFTER 		10000
#define COMMON_CHANNEL		0x2904
#define DATA_CHANNEL		0x0503

#endif

