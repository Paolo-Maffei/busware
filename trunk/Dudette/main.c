/*****************************************************************************
*
* AVRPROG compatible boot-loader
* Version  : 0.85 (Dec. 2008)
* Compiler : avr-gcc 4.1.2 / avr-libc 1.4.6
* size     : depends on features and startup ( minmal features < 512 words)
* by       : Martin Thomas, Kaiserslautern, Germany
*            eversmith@heizung-thomas.de
*            Additional code and improvements contributed by:
*           - Uwe Bonnes
*           - Bjoern Riemer
*           - Olaf Rempel
*
* License  : 
*	     Copyright (c) 2006-2008 M. Thomas, U. Bonnes, O. Rempel
*            Free to use. You have to mention the copyright
*            owners in source-code and documentation of derived
*            work. No warranty! (Yes, you can insert the BSD
*            license here)
*
* Tested with ATmega8, ATmega16, ATmega162, ATmega32, ATmega324P,
*             ATmega644, ATmega644P, ATmega128, AT90CAN128
*
* - Initial versions have been based on the Butterfly bootloader-code
*   by Atmel Corporation (Authors: BBrandal, PKastnes, ARodland, LHM)
*
****************************************************************************
*
*  See the makefile and readme.txt for information on how to adapt 
*  the linker-settings to the selected Boot Size (BOOTSIZE=xxxx) and 
*  the MCU-type. Other configurations futher down in this file.
*
****************************************************************************/
/*
	TODOs:
	- check lock-bits set
	- __bad_interrupt still linked even with modified 
	  linker-scripts which needs a default-handler,
	  "wasted": 3 words for AVR5 (>8kB), 2 words for AVR4
	- Check watchdog-disable-function in avr-libc.
*/

#include "board.h"
#include <avr/eeprom.h>
#include <string.h>
#ifdef BOOT_RADIO
#include "seriallink.h"
#endif

#ifdef RADIO_CRYPT
#include "rsa.h"
#define RSA_MAX_LEN (128/8)
unsigned char AES_KEY[RSA_MAX_LEN]   = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
unsigned char RSA_PUBL[RSA_MAX_LEN]  = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
unsigned char RSA_PRIV[RSA_MAX_LEN]  = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
unsigned char RSA_SPUBL[RSA_MAX_LEN] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
unsigned char rsa_tmp[3*RSA_MAX_LEN];
#define rsa_s (rsa_tmp+(2*RSA_MAX_LEN))
#endif

uint8_t start_me = 0;

#define EE_MAC_ADDR (uint8_t*)(E2END+1-6)
#define EE_RSA_PRIV (uint8_t*)(EE_MAC_ADDR-16)
#define EE_RSA_PUBL (uint8_t*)(EE_RSA_PRIV-16)
#define EE_RSA_SPUBL (uint8_t*)(EE_RSA_PUBL-16)

/* MCU frequency */
#ifndef F_CPU
#error "please define F_CPU!"
#endif

/* Device-Type:
   For AVRProg the BOOT-option is prefered 
   which is the "correct" value for a bootloader.
   avrdude may only detect the part-code for ISP */
#define DEVTYPE     DEVTYPE_BOOT
//#define DEVTYPE     DEVTYPE_ISP

/*
 * Define if Watchdog-Timer should be disable at startup
 */
#define DISABLE_WDT_AT_STARTUP

/*
 * Watchdog-reset is issued at exit 
 * define the timeout-value here (see avr-libc manual)
 */
#define EXIT_WDT_TIME   WDTO_250MS

/*
 * enable/disable readout of fuse and lock-bits
 * (AVRPROG has to detect the AVR correctly by device-code
 * to show the correct information).
 */
//#define ENABLEREADFUSELOCK

/* enable/disable write of lock-bits
 * WARNING: lock-bits can not be reseted by bootloader (as far as I know)
 * Only protection no unprotection, "chip erase" from bootloader only
 * clears the flash but does no real "chip erase" (this is not possible
 * with a bootloader as far as I know)
 * Keep this undefined!
 */
//#define WRITELOCKBITS

/*
 * define the following if the bootloader should not output
 * itself at flash read (will fake an empty boot-section)
 */
#define READ_PROTECT_BOOTLOADER

#define VERSION_HIGH '0'
#define VERSION_LOW  '8'

#define GET_LOCK_BITS           0x0001
#define GET_LOW_FUSE_BITS       0x0000
#define GET_HIGH_FUSE_BITS      0x0003
#define GET_EXTENDED_FUSE_BITS  0x0002

#include <stdint.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "chipdef.h"

#ifdef UART_DOUBLESPEED
// #define UART_CALC_BAUDRATE(baudRate) (((F_CPU*10UL) / ((baudRate) *8UL) +5)/10 -1)
#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)((F_CPU) + ((uint32_t)baudRate * 4UL)) / ((uint32_t)(baudRate) * 8UL) - 1)
#else
// #define UART_CALC_BAUDRATE(baudRate) (((F_CPU*10UL) / ((baudRate)*16UL) +5)/10 -1)
#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)((F_CPU) + ((uint32_t)baudRate * 8UL)) / ((uint32_t)(baudRate) * 16UL) - 1)
#endif

uint8_t gBuffer[SPM_PAGESIZE];

#if defined(BOOTLOADERHASNOVECTORS)
#warning "This Bootloader does not link interrupt vectors - see makefile"
/* make the linker happy - it wants to see __vector_default */
// void __vector_default(void) { ; }
void __vector_default(void) { ; }
#endif

// Function prototypes
static void (*_jump_to_app)(void) = 0x0000;
static void (*sendchar)(uint8_t data);
static uint8_t (*recvchar)(void);

static uint16_t led_pattern = 0xaaaa;
#ifdef LED_PORT
static uint16_t led_counter = 1;
static uint8_t led_step     = 0;
#endif

void blink(void) {
#ifdef LED_PORT
	if (--led_counter)
		return;
	led_counter = 15000;

	if ((led_pattern & _BV(led_step++))) {
		LED_PORT |= _BV(LED_PIN);
	} else {
		LED_PORT &= ~_BV(LED_PIN);
	}
	led_step &= 15;
#endif
}

#ifdef BOOT_SERIAL
static void serial_sendchar(uint8_t data) {
	  while (!(UART_STATUS & (1<<UART_TXREADY)));
	  UART_DATA = data;
}

static uint8_t serial_recvchar(void) {
          while (!(UART_STATUS & (1<<UART_RXREADY)))
		blink();
	  return UART_DATA;
}
#endif

#ifdef BOOT_RADIO
static void radio_sendchar(uint8_t data) {
          while (!slink_avail());
          slink_put( data );
}

static uint8_t radio_recvchar(void) {
          while (!slink_elements())
		blink();
          return slink_get();
}
#endif

static inline void eraseFlash(void)
{
	// erase only main section (bootloader protection)
	uint32_t addr = 0;
	boot_spm_busy_wait();
	while (APP_END > addr) {
		boot_page_erase_safe(addr);		// Perform page erase
		boot_spm_busy_wait();		// Wait until the memory is erased.
		addr += SPM_PAGESIZE;
	}
	boot_rww_enable();
}

static inline void recvBuffer(pagebuf_t size)
{
	pagebuf_t cnt;
	uint8_t *tmp = gBuffer;

	for (cnt = 0; cnt < sizeof(gBuffer); cnt++) {
		*tmp++ = (cnt < size) ? recvchar() : 0xFF;
	}
}

static inline uint16_t writeFlashPage(uint16_t waddr, pagebuf_t size)
{
	uint32_t pagestart = (uint32_t)waddr<<1;
	uint32_t baddr = pagestart;
	uint16_t data;
	uint8_t *tmp = gBuffer;

	boot_spm_busy_wait();

	do {
		data = *tmp++;
		data |= *tmp++ << 8;
		boot_page_fill(baddr, data);	// call asm routine.

		baddr += 2;			// Select next word in memory
		size -= 2;			// Reduce number of bytes to write by two
	} while (size);				// Loop until all bytes written

	boot_page_write_safe(pagestart);
	boot_spm_busy_wait();
	boot_rww_enable();		// Re-enable the RWW section

	return baddr>>1;
}

static inline uint16_t writeEEpromPage(uint16_t address, pagebuf_t size)
{
	uint8_t *tmp = gBuffer;

	do {
		if (address<(uint16_t)EE_RSA_PRIV)
			eeprom_write_byte( (uint8_t*)address, *tmp );
		tmp++;
		address++;			// Select next byte
		size--;				// Decreas number of bytes to write
	} while (size);				// Loop until all bytes written

	// eeprom_busy_wait();

	return address;
}

static inline uint16_t readFlashPage(uint16_t waddr, pagebuf_t size)
{
	uint32_t baddr = (uint32_t)waddr<<1;
	uint16_t data;

	boot_spm_busy_wait();

	do {
#ifndef READ_PROTECT_BOOTLOADER
#warning "Bootloader not read-protected"
#if defined(RAMPZ)
		data = pgm_read_word_far(baddr);
#else
		data = pgm_read_word_near(baddr);
#endif
#else
		// don't read bootloader
		if ( baddr < APP_END ) {
#if defined(RAMPZ)
			data = pgm_read_word_far(baddr);
#else
			data = pgm_read_word_near(baddr);
#endif
		}
		else {
			data = 0xFFFF; // fake empty
		}
#endif
		sendchar(data);			// send LSB
		sendchar((data >> 8));		// send MSB
		baddr += 2;			// Select next word in memory
		size -= 2;			// Subtract two bytes from number of bytes to read
	} while (size);				// Repeat until block has been read

	return baddr>>1;
}

static inline uint16_t readEEpromPage(uint16_t address, pagebuf_t size)
{
	do {
		if ((address<(uint16_t)EE_RSA_PRIV) || (address>=(uint16_t)EE_MAC_ADDR)) {
			sendchar( eeprom_read_byte( (uint8_t*)address ) );
		} else {
			sendchar( 0xff );
		}
		address++;
		size--;				// Decrease number of bytes to read
	} while (size);				// Repeat until block has been read

	return address;
}

#if defined(ENABLEREADFUSELOCK)
static uint8_t read_fuse_lock(uint16_t addr)
{
	uint8_t mode = (1<<BLBSET) | (1<<SPMEN);
	uint8_t retval;

	asm volatile
	(
		"movw r30, %3\n\t"		/* Z to addr */ \
		"sts %0, %2\n\t"		/* set mode in SPM_REG */ \
		"lpm\n\t"			/* load fuse/lock value into r0 */ \
		"mov %1,r0\n\t"			/* save return value */ \
		: "=m" (SPM_REG),
		  "=r" (retval)
		: "r" (mode),
		  "r" (addr)
		: "r30", "r31", "r0"
	);
	return retval;
}
#endif

static void send_boot(void)
{
	sendchar('A');
	sendchar('V');
	sendchar('R');
	sendchar('B');
	sendchar('O');
	sendchar('O');
	sendchar('T');
}


static void jump_to_app( void ) {
	led_pattern = 0x2a;
	while (pgm_read_byte(0) == 0xff)
		blink();
	_jump_to_app();
}

void display_hex(uint16_t h, int8_t pad) {
     char buf[5];
     char *s;
     
     int8_t i=5;
     
     buf[--i] = 0;
     do {
	  uint8_t m = h%16;
	  buf[--i] = (m < 10 ? '0'+m : 'A'+m-10);
	  h /= 16;
	  pad--;
     } while(h);
     
     while(--pad >= 0 && i > 0)
	  buf[--i] = '0';


     s = buf+i;
     
     while(*s)
	  sendchar(*s++);
}

int main(void)
{
	uint16_t address = 0;
	uint8_t device = 0, val;

#ifdef LED_PORT
	// LED
	LED_DDR  |= _BV(LED_PIN);
#endif

#ifdef ALWAYSLED_PORT
	// LED
	ALWAYSLED_DDR  |= _BV(ALWAYSLED_PIN);
	ALWAYSLED_PORT |= _BV(ALWAYSLED_PIN);
#endif

#ifdef DISABLE_WDT_AT_STARTUP
#ifdef WDT_OFF_SPECIAL
#warning "using target specific watchdog_off"
	bootloader_wdt_off();
#else
	cli();
	wdt_reset();
	wdt_disable();
#endif
#endif
	
	// is Flash empty?
       	start_me = (pgm_read_byte(0) == 0xff);

/********************************/
#ifdef BOOT_SERIAL
/********************************/

#ifdef UART_STACKED
	SWITCH_DDR  &= ~_BV(SWITCH_PIN);	// set as Input
#ifdef SWITCH_LOWACTIVE
	SWITCH_PORT |= _BV(SWITCH_PIN);		// pull
	_delay_ms(100);				// allow possible external C to charge
#endif
#endif
	BOOT_DDR  &= ~_BV(BOOT_PIN);		// set as Input
#ifdef BOOT_LOWACTIVE
	BOOT_PORT |= _BV(BOOT_PIN);		// pull
	_delay_ms(100);				// allow possible external C to charge
#endif

#ifdef BOOT_LOWACTIVE
	if (!bit_is_set(BOOT_IN, BOOT_PIN)) {
#else
	if (bit_is_set(BOOT_IN, BOOT_PIN)) {
#endif
		// Set baud rate
		UART_BAUD_HIGH = (UART_CALC_BAUDRATE(BAUDRATE)>>8) & 0xFF;
		UART_BAUD_LOW = (UART_CALC_BAUDRATE(BAUDRATE) & 0xFF);

#ifdef UART_DOUBLESPEED
		UART_STATUS = ( 1<<UART_DOUBLE );
#endif

		UART_CTRL = UART_CTRL_DATA;
		UART_CTRL2 = UART_CTRL2_DATA;

// is it really us bootloading - if stacked?
#ifdef UART_STACKED
#ifdef SWITCH_LOWACTIVE
	if (bit_is_set(SWITCH_IN, SWITCH_PIN)) {
#else
	if (!bit_is_set(SWITCH_IN, SWITCH_PIN)) {
#endif
		// no! we just forwarding packets ...

		// assign same format to UART1
		UART1_BAUD_HIGH = UART_BAUD_HIGH;
		UART1_BAUD_LOW  = UART_BAUD_LOW;
		UART1_STATUS    = UART_STATUS;
                UART1_CTRL      = UART_CTRL;
                UART1_CTRL2     = UART_CTRL2;

		uint8_t buf1[256], buf2[256], pos1, pos2;

		pos1 = 0;
		pos2 = 0;

		while(1) {

			// received a byte from RPi
          		if ((UART_STATUS & (1<<UART_RXREADY))) {
			  	buf1[pos1++] = UART_DATA;
			}
	  		if(pos1 && (UART1_STATUS & (1<<UART1_TXREADY)))
	  		  	UART1_DATA = buf1[--pos1];

			// received a byte from module above
          		if ((UART1_STATUS & (1<<UART1_RXREADY))) {
			  	buf2[pos2++] = UART1_DATA;
			}
	  		if(pos2 && (UART_STATUS & (1<<UART_TXREADY)))
	  		  	UART_DATA = buf2[--pos2];

		}
	}
#endif
  		sendchar = serial_sendchar;
  		recvchar = serial_recvchar;
	
		BOOT_DDR  &= ~_BV(BOOT_PIN);		// set to default		
		BOOT_PORT &= ~_BV(BOOT_PIN);		// set to default		

		led_pattern = 0xff00;

		start_me = 1;
	}

#endif // BOOT_SERIAL

/********************************/
#ifdef BOOT_RADIO
/********************************/
        struct apkt pkt;
        uint16_t to = 0;

#ifndef RADIO_ALWAYS
	BOOT_RADIO_DDR  &= ~_BV(BOOT_RADIO_PIN);		// set as Input
#ifdef BOOT_RADIO_LOWACTIVE
	BOOT_RADIO_PORT |= _BV(BOOT_RADIO_PIN);		// pull
	_delay_ms(100);				// allow possible external C to charge
	if (!bit_is_set(BOOT_RADIO_IN, BOOT_RADIO_PIN)) {
#else
	if (bit_is_set(BOOT_RADIO_IN, BOOT_RADIO_PIN)) {
#endif
#endif

	slink_init(RADIO_CHANNEL);
	pkt.seq = 'B';
	pkt.len = 6; // length of MAC address
	for( to=0; to<6; to++)
	  pkt.data[to] = eeprom_read_byte( EE_MAC_ADDR+to );

#ifdef RADIO_CRYPT
	// add our public key ...
	uint8_t have_skey = 0;
	for( to=0; to<RSA_MAX_LEN; to++) {
	  pkt.data[6+to] = eeprom_read_byte( EE_RSA_PUBL+to );
	  RSA_PUBL[to]   = eeprom_read_byte( EE_RSA_PUBL+to );
	  RSA_PRIV[to]   = eeprom_read_byte( EE_RSA_PRIV+to );
	  RSA_SPUBL[to]  = eeprom_read_byte( EE_RSA_SPUBL+to );

	  // if all bytes are 0xff (deleted) - assume no server key is loaded
	  if (RSA_SPUBL[to] != 0xff)
	    have_skey = 1;;
        }
	pkt.len += RSA_MAX_LEN;
#endif

	pkt.start = 1;
	pkt.plen  = pkt.len+1; 

	slink_send( &pkt ); // send own MAC
	to = RESEND_AFTER;
	while (to--) {
		if (slink_recv(&pkt)==SL_OK) {
			// does the packet look like a 'b'+<channelnumber> ?
			if (pkt.seq=='b' && ((pkt.len==2)||(pkt.len>=18))) {
				to = (pkt.data[0]<<8) | pkt.data[1];

#ifdef RADIO_CRYPT
				if (have_skey && (pkt.len<34)) {
					// signed message required ... giving up 
					break;
				}

				// are there any keys sent?
				if (pkt.len>=18) {

					// was there a signature sent?
					if (have_skey && (pkt.len>=34)) {
		                                rsa_decrypt(RSA_MAX_LEN,&(pkt.data[18]),3,RSA_SPUBL,rsa_s,rsa_tmp);
						if (memcmp( &(pkt.data[2]), &(pkt.data[18]), RSA_MAX_LEN )!=0) {
							// signature is invalid!
							break;
						}
					}

                                        memcpy(AES_KEY,&(pkt.data[2]),RSA_MAX_LEN);
                                	rsa_encrypt(RSA_MAX_LEN,AES_KEY,RSA_PRIV,RSA_PUBL,rsa_s,rsa_tmp);
					slink_init(to);
					slink_crypt( AES_KEY );

				} else
#endif
					slink_init(to);

                		sendchar = radio_sendchar;
                		recvchar = radio_recvchar;
				led_pattern = 0xaaaa;
				start_me = 1;
			}
			break;
		}
   	};

#ifndef RADIO_ALWAYS
   	};
	BOOT_RADIO_DDR  &= ~_BV(BOOT_RADIO_PIN);		// set to default		
	BOOT_RADIO_PORT &= ~_BV(BOOT_RADIO_PIN);		// set to default		
#endif
#endif

	if (!start_me) {
#ifdef BOOT_RADIO
		slink_done();
#endif
#ifdef LED_PORT
	        LED_PORT &= ~_BV(LED_PIN);
	        LED_DDR  &= ~_BV(LED_PIN);
#endif
		jump_to_app();			// Jump to application sector
	}


	if (!sendchar || !recvchar) {
		// ups?! no method for bootloading choosen
		led_pattern = 0x0a;
		while (1) 
			blink();
	}

// if we have stackable modules support we need to find the module actually bootloading
// the others just forward packets
#ifdef BOOT_CHAIN_PORT

	BOOT_CHAIN_DDR  &= ~_BV(BOOT_CHAIN_PIN);		// set as Input
#ifdef BOOT_CHAIN_LOWACTIVE
	BOOT_CHAIN_PORT |= _BV(BOOT_CHAIN_PIN);		// pull
	_delay_ms(100);				// allow possible external C to charge
#endif

#ifdef BOOT_CHAIN_LOWACTIVE
	if (bit_is_set(BOOT_CHAIN_IN, BOOT_CHAIN_PIN)) {
#else
	if (!bit_is_set(BOOT_CHAIN_IN, BOOT_CHAIN_PIN)) {
#endif

          // we end here if bootloading was requested but CHAIN_BUTTON is NOT pressed
          // ... so we we are just forwarding packets


	  UBRR1  = UBRR0;
	  UCSR1A = UCSR0A;
	  UCSR1B = UCSR0B;
	  UCSR1C = UCSR0C;

	  while (1) {
	
            while (!(UART_STATUS & (1<<UART_TXREADY)));
              UART_DATA = data;
	    }

            while (!(UART_STATUS & (1<<UART_RXREADY)))
              return UART_DATA;
            }

        }

#endif

	for(;;) {

#ifdef LED_PORT
		LED_PORT |= _BV(LED_PIN);
#endif
		val = recvchar();
#ifdef LED_PORT
		LED_PORT &= ~_BV(LED_PIN);
#endif

		// Autoincrement?
		if (val == 'a') {
			sendchar('Y');			// Autoincrement is quicker

		//write address
		} else if (val == 'A') {
			address = recvchar();		//read address 8 MSB
			address = (address<<8) | recvchar();
			sendchar('\r');

		// Buffer load support
		} else if (val == 'b') {
			sendchar('Y');					// Report buffer load supported
			sendchar((sizeof(gBuffer) >> 8) & 0xFF);	// Report buffer size in bytes
			sendchar(sizeof(gBuffer) & 0xFF);

		// Start buffer load
		} else if (val == 'B') {
			pagebuf_t size;
			size = recvchar() << 8;				// Load high byte of buffersize
			size |= recvchar();				// Load low byte of buffersize
			val = recvchar();				// Load memory type ('E' or 'F')
			recvBuffer(size);

			if (device == DEVTYPE) {
				if (val == 'F') {
					address = writeFlashPage(address, size);
				} else if (val == 'E') {
					address = writeEEpromPage(address, size);
				}
				sendchar('\r');
			} else {
				sendchar(0);
			}

		// Block read
		} else if (val == 'g') {
			pagebuf_t size;
			size = recvchar() << 8;				// Load high byte of buffersize
			size |= recvchar();				// Load low byte of buffersize
			val = recvchar();				// Get memtype

			if (val == 'F') {
				address = readFlashPage(address, size);
			} else if (val == 'E') {
				address = readEEpromPage(address, size);
			}

		// Chip erase
 		} else if (val == 'e') {
			if (device == DEVTYPE) {
				eraseFlash();
			}
			sendchar('\r');

		// Exit upgrade
		} else if (val == 'E') {
			wdt_enable(EXIT_WDT_TIME); // Enable Watchdog Timer to give reset
			sendchar('\r');

#ifdef WRITELOCKBITS
#warning "Extension 'WriteLockBits' enabled"
		// TODO: does not work reliably
		// write lockbits
		} else if (val == 'l') {
			if (device == DEVTYPE) {
				// write_lock_bits(recvchar());
				boot_lock_bits_set(recvchar());	// boot.h takes care of mask
				boot_spm_busy_wait();
			}
			sendchar('\r');
#endif
		// Enter programming mode
		} else if (val == 'P') {
			sendchar('\r');

		// Leave programming mode
		} else if (val == 'L') {
			sendchar('\r');

		// return programmer type
		} else if (val == 'p') {
			sendchar('S');		// always serial programmer

#ifdef ENABLEREADFUSELOCK
#warning "Extension 'ReadFuseLock' enabled"
		// read "low" fuse bits
		} else if (val == 'F') {
			sendchar(read_fuse_lock(GET_LOW_FUSE_BITS));

		// read lock bits
		} else if (val == 'r') {
			sendchar(read_fuse_lock(GET_LOCK_BITS));

		// read high fuse bits
		} else if (val == 'N') {
			sendchar(read_fuse_lock(GET_HIGH_FUSE_BITS));

		// read extended fuse bits
		} else if (val == 'Q') {
			sendchar(read_fuse_lock(GET_EXTENDED_FUSE_BITS));
#endif

		// Return device type
		} else if (val == 't') {
			sendchar(DEVTYPE);
			sendchar(0);

		// clear and set LED ignored
		} else if ((val == 'x') || (val == 'y')) {
			recvchar();
			sendchar('\r');

		// set device
		} else if (val == 'T') {
			device = recvchar();
			sendchar('\r');

		// Return software identifier
		} else if (val == 'S') {
			send_boot();

		// Return Software Version
		} else if (val == 'V') {
			sendchar(VERSION_HIGH);
			sendchar(VERSION_LOW);

		// Return Signature Bytes (it seems that 
		// AVRProg expects the "Atmel-byte" 0x1E last
		// but shows it first in the dialog-window)
		} else if (val == 's') {
			sendchar(SIG_BYTE3);
			sendchar(SIG_BYTE2);
			sendchar(SIG_BYTE1);

/*
		} else if (val == 'm') {
		        for( uint8_t i=0; i<6; i++)
          			sendchar( eeprom_read_byte( EE_MAC_ADDR+i ));
*/

		} else if (val == 'M') {
		        for( uint8_t i=0; i<5; i++) {
          			display_hex( eeprom_read_byte( EE_MAC_ADDR+i ), 2);
				sendchar( ':' );
			}	
          		display_hex( eeprom_read_byte( EE_MAC_ADDR+5 ), 2);

/*
		} else if (val == 'k') {
			for( uint8_t i=0; i<RSA_MAX_LEN; i++) 
          			sendchar( eeprom_read_byte( EE_RSA_PUBL+i ));

*/
		} else if (val == 'K') {
			for( uint8_t i=0; i<15; i++) { 
          			display_hex( eeprom_read_byte( EE_RSA_PUBL+i ), 2);
				sendchar( ':' );
			}
          		display_hex( eeprom_read_byte( EE_RSA_PUBL+15 ), 2);

		/* ESC */
		} else if(val != 0x1b) {
			sendchar('?');
		}
	}
	return 0;
}
