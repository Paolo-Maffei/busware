/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the DualVirtualSerial demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "board.h"
#include "Airdude.h"
#include "seriallink.h"
#include "rsa.h"

#define DEBUG 1

#ifdef NO_PGM
#undef DEBUG
#endif

#if DEBUG
#include <stdio.h>
#include <avr/pgmspace.h>
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...)
#endif

// change this KEY!
// make use of: hexdump -v -n "16" -e '1/1 "0x%02x,"' /dev/urandom
unsigned char AES_KEY[16] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
unsigned char RSA_PUBL[16] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };
unsigned char RSA_PRIV[16] = { 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6 };

unsigned char PROGMEM private128[] = { 0xa0, 0x63, 0x0a, 0x93, 0x0f, 0x86, 0xa5, 0x1a, 0x63, 0x28, 0xf0, 0x92, 0x89, 0xf8, 0x47, 0x63 };
unsigned char PROGMEM public128[]  = { 0xf0, 0x94, 0x8f, 0xdc, 0x97, 0x49, 0xf7, 0xa9, 0x85, 0x1a, 0x12, 0x04, 0x79, 0x04, 0x6b, 0x21 };

#define RSA_MAX_LEN (128/8)
unsigned char cryptdata[RSA_MAX_LEN];

unsigned char rsa_tmp[3*RSA_MAX_LEN];
#define rsa_s (rsa_tmp+(2*RSA_MAX_LEN))

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the first CDC interface,
 *  which sends strings to the host for each joystick movement.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial1_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = 0,
				.DataINEndpoint           =
					{
						.Address          = CDC1_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC1_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC1_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the second CDC interface,
 *  which echos back all received data from the host.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial2_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = 2,
				.DataINEndpoint           =
					{
						.Address          = CDC2_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC2_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC2_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},

			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */

int usb_putchar (char data, FILE * stream) {
        CDC_Device_SendByte(&VirtualSerial2_CDC_Interface, data);
        return 0;
}

static uint8_t mode = 0;

int main(void) {
	int c, do_crypt;
	uint16_t loop;
        struct apkt pkt;

	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	slink_init( COMMON_CHANNEL );
	mode = 0;

	for (;;) {

		if (mode) {

                // is data coming from USB?
                while (slink_avail()) {
                        c = CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);
                        if (c>=0) {
                                slink_put( c&0xff );
                                continue;
                        }
                        break;
                }

		// do we have received bytes from air?
                while (slink_elements())
                       	CDC_Device_SendByte(&VirtualSerial1_CDC_Interface, slink_get());

 
		} else {

			// dummy read USB data
			CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);

	
                if (slink_recv(&pkt)==SL_OK) {
                        PRINTF( "mode 0: recved pkt\n");
                        // does the packet look like a 'B'+<MAC address> ?
                        if (pkt.seq=='B' && (pkt.len==6||pkt.len==22)) {
                                PRINTF( "%x:%x:%x:%x:%x:%x is booting ...\n", pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3], pkt.data[4], pkt.data[5] );

				do_crypt = 0;
				if (pkt.len==22)
					do_crypt = 1;

                                // catch him!
                                pkt.seq = 'b';
                                pkt.len = 2; // length of channel number
                                pkt.data[0] = (DATA_CHANNEL>>8);
                                pkt.data[1] = DATA_CHANNEL & 0xff;

				// was there a public key?
				if (do_crypt) {
					memcpy(cryptdata,AES_KEY,RSA_MAX_LEN);
				  	for (uint8_t i=0;i<RSA_MAX_LEN;i++) PRINTF( "%x ", cryptdata[i] ); PRINTF( "\n" );
        			  	rsa_decrypt(RSA_MAX_LEN,cryptdata,3,&(pkt.data[6]),rsa_s,rsa_tmp); 
					for (uint8_t i=0;i<RSA_MAX_LEN;i++) {
						pkt.data[2+i] = cryptdata[i];
						PRINTF( "%x ", cryptdata[i] );
					}
					PRINTF( "\n" );
					pkt.len += RSA_MAX_LEN;

					// generate signature
					memcpy_P(RSA_PUBL,public128,RSA_MAX_LEN);
					memcpy_P(RSA_PRIV,private128,RSA_MAX_LEN);
        				rsa_encrypt(RSA_MAX_LEN,cryptdata,RSA_PRIV,RSA_PUBL,rsa_s,rsa_tmp);
					PRINTF( "Signature: " ); 
					for (uint8_t i=0;i<RSA_MAX_LEN;i++) {
						pkt.data[18+i] = cryptdata[i];
						PRINTF( "%x ", cryptdata[i] ); 
					}
					PRINTF( "\n" );
					pkt.len += RSA_MAX_LEN;

				}

                                pkt.plen = pkt.len+1; // length of payload
                                pkt.start = 1; // pkt index
                                slink_send( &pkt );

                                PRINTF( "Entering mode 1\n" );
                                slink_init(DATA_CHANNEL);
				if (do_crypt)
					slink_crypt(AES_KEY);

                                mode = 1;
                        }
                }



		}


		switch( CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface) ) {
			case 'c':
			case 'C':
        			memcpy(cryptdata,AES_KEY,RSA_MAX_LEN);
				PRINTF( "Given Data: " ); for (uint8_t i=0;i<RSA_MAX_LEN;i++) PRINTF( "%x ", cryptdata[i] ); PRINTF( "\n" );

				memcpy_P(RSA_PUBL,public128,RSA_MAX_LEN);
				memcpy_P(RSA_PRIV,private128,RSA_MAX_LEN);
        			rsa_encrypt(RSA_MAX_LEN,cryptdata,RSA_PRIV,RSA_PUBL,rsa_s,rsa_tmp);
				PRINTF( "Signature: " ); for (uint8_t i=0;i<RSA_MAX_LEN;i++) PRINTF( "%x ", cryptdata[i] ); PRINTF( "\n" );

				memcpy_P(RSA_PUBL,public128,RSA_MAX_LEN);
        			rsa_decrypt(RSA_MAX_LEN,cryptdata,3,RSA_PUBL,rsa_s,rsa_tmp); 
				for (uint8_t i=0;i<RSA_MAX_LEN;i++) PRINTF( "%x ", cryptdata[i] ); PRINTF( "\n" );

				break;
			case 'R':
			case 'r':
				PRINTF( "Entering mode 0\n" );
				slink_init( COMMON_CHANNEL );
				mode = 0;
				break;
			default:
				break;
		};

		CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);
		CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
		USB_USBTask();
	
		// randomize the AES key:
	//	if (loop++==0)
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();

        fdevopen(&usb_putchar,NULL);
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial1_CDC_Interface);
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial2_CDC_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial1_CDC_Interface);
	CDC_Device_ProcessControlRequest(&VirtualSerial2_CDC_Interface);
//	printf( "EVENT_USB_Device_ControlRequest\r\n" );
}

