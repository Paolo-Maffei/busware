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

unsigned char PROGMEM private64e3[] = { 0xa0, 0x63, 0x0a, 0x93, 0x0f, 0x86, 0xa5, 0x1a, 0x63, 0x28, 0xf0, 0x92, 0x89, 0xf8, 0x47, 0x63 };
unsigned char PROGMEM public64e3[]  = { 0xf0, 0x94, 0x8f, 0xdc, 0x97, 0x49, 0xf7, 0xa9, 0x85, 0x1a, 0x12, 0x04, 0x79, 0x04, 0x6b, 0x21 };

//unsigned char PROGMEM private64e3[] = { 0x8B,0x35,0xEB,0x99,0x91,0x07,0xB6,0x0B};
//unsigned char PROGMEM public64e3[]  = { 0xD0,0xD0,0xE1,0x68,0x28,0xC7,0x35,0x59};

/* just random data for testeting */
unsigned char PROGMEM CONSTANT_DATA[]= {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x5A,0x59,0xE9,0x71,0x1A,0xCB,0x01,0x11,0xDF,0x92,0x8E,0xF4,0x7B,0xAD,0xD8,0x69
};

#define RSA_MAX_LEN (128/8)
unsigned char cryptdata[RSA_MAX_LEN];
//unsigned char public_key[RSA_MAX_LEN];
//unsigned char private_key[RSA_MAX_LEN];
//unsigned int  public_exponent;

unsigned char rsa_tmp[3*RSA_MAX_LEN];
#define rsa_s (rsa_tmp+(2*RSA_MAX_LEN))
//unsigned char rsa_s[RSALEN];


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
	int c;
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
                        if (pkt.seq=='B' && pkt.len==6) {
                                PRINTF( "%x:%x:%x:%x:%x:%x is booting ...\n", pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3], pkt.data[4], pkt.data[5] );

                                // catch him!
                                pkt.seq = 'b';
                                pkt.len = 2; // length of channel number
                                pkt.data[0] = (DATA_CHANNEL>>8);
                                pkt.data[1] = DATA_CHANNEL & 0xff;
                                slink_send( &pkt );

                                PRINTF( "Entering mode 1\n" );
                                slink_init(DATA_CHANNEL);
                                mode = 1;
                        }
                }



		}


		switch( CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface) ) {
			case 'c':
			case 'C':
				PRINTF( "A\n" );
        			memcpy_P(cryptdata,CONSTANT_DATA,sizeof(cryptdata));
				for (uint8_t i=0;i<RSA_MAX_LEN;i++)
					PRINTF( "%x ", cryptdata[i] );
				PRINTF( "\n" );

        			rsa_decrypt_P(sizeof(public64e3),cryptdata,3,public64e3,rsa_s,rsa_tmp); 
				for (uint8_t i=0;i<RSA_MAX_LEN;i++)
					PRINTF( "%x ", cryptdata[i] );
				PRINTF( "\n" );

        			rsa_encrypt_P(sizeof(public64e3),cryptdata ,private64e3,public64e3,rsa_s,rsa_tmp);
				for (uint8_t i=0;i<RSA_MAX_LEN;i++)
					PRINTF( "%x ", cryptdata[i] );
				PRINTF( "\n" );

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

