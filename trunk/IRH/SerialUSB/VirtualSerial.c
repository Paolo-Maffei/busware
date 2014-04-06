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
 *  Main source file for the VirtualSerial demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "VirtualSerial.h"
#include <stddef.h>
#include <avr/pgmspace.h> 
#include <util/delay.h> 
#include "usart_driver.h"

#define USART USARTE0
USART_data_t USART_data;


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = 0,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

uint16_t loop;
uint8_t configured = 0;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	GlobalInterruptEnable();

	uint8_t sending = 0;

	for (;;) {

		while (1) {
                	int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
			if (ReceivedByte < 0) 
				break;

			if (!configured) continue;

			if (!sending) {
				sending = 1;
			}

	        	PORTA.OUTCLR = PIN4_bm;
                	while(!USART_IsTXDataRegisterEmpty(&USART));
                	USART_PutChar(&USART, ReceivedByte & 0xff);
		}

		if (sending) {
			USART_ClearTXComplete(&USART);
               		while(!USART_IsTXComplete(&USART));
			sending = 0;	
		}

                Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

                /* Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
                 * until it completes as there is a chance nothing is listening and a lengthy timeout could occur */

		if (configured && Endpoint_IsINReady()) {
			uint8_t maxbytes = CDC_TXRX_EPSIZE;
                	while (USART_RXBufferData_Available(&USART_data) && maxbytes-->0) {
	        		PORTA.OUTCLR = PIN4_bm;
                        	uint8_t b = USART_RXBuffer_GetByte(&USART_data);
				CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b);
                	}
                }

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	        PORTA.OUTSET = PIN4_bm;

		if (loop++) continue;
		if (!configured) continue;

		PORTA.OUTTGL = PIN3_bm;
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;

        // EXOR Gatter
	PORTE.DIRSET = PIN0_bm;
	PORTE.DIRSET = PIN1_bm;

//	PORTE.OUTSET = PIN0_bm; // V1.0
	PORTE.OUTCLR = PIN0_bm; // V1.1
	PORTE.OUTCLR = PIN1_bm;

	// LED
	PORTA.DIRSET = PIN2_bm; // Red
	PORTA.DIRSET = PIN3_bm; // Green
	PORTA.DIRSET = PIN4_bm; // Blue

	// LEDS OFF
	PORTA.OUTSET = PIN2_bm;
	PORTA.OUTCLR = PIN3_bm;
	PORTA.OUTSET = PIN4_bm;

	/* Hardware Initialization */

	// USART RX/TX 1
	/* PIN3 (TXD0) as output. */
        PORTE.DIRSET = PIN3_bm;
        /* PC2 (RXD0) as input. */
        PORTE.DIRCLR = PIN2_bm;

        USART_InterruptDriver_Initialize(&USART_data, &USART, USART_DREINTLVL_OFF_gc);
        USART_RxdInterruptLevel_Set(USART_data.usart, USART_RXCINTLVL_LO_gc);

        /* Enable global interrupts. */
        sei();

	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
        uint8_t pmode = 0;
        uint8_t csize = 0;
        uint8_t twosb = 0;
        int8_t bscale = 4;
        uint8_t bsel  = 12;

        switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
        {
                case CDC_PARITY_Odd:
                        pmode = USART_PMODE_ODD_gc; 
                        break;
                case CDC_PARITY_Even:
                        pmode = USART_PMODE_EVEN_gc; 
                        break;
		default:
                        pmode = USART_PMODE_DISABLED_gc; 

        }

        if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
          twosb = 1;

        switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
        {
                case 6:
                        csize = USART_CHSIZE_6BIT_gc; 
                        break;
                case 7:
                        csize = USART_CHSIZE_7BIT_gc; 
                        break;
                case 8:
                        csize = USART_CHSIZE_8BIT_gc; 
                        break;
		default:
			return;
        }

        switch (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS)
        {
                case 300:
                        bsel   = 12;
                        bscale = 9;
                        break;
                case 600:
                        bsel   = 12;
                        bscale = 8;
                        break;
                case 1200:
                        bsel   = 12;
                        bscale = 7;
                        break;
                case 2400:
                        bsel   = 12;
                        bscale = 6;
                        break;
                case 4800:
                        bsel   = 12;
                        bscale = 5;
                        break;
                case 9600:
                        bsel   = 12;
                        bscale = 4;
                        break;
                case 14400:
                        bsel   = 138;
                        bscale = 0;
                        break;
                case 19200:
                        bsel   = 12;
                        bscale = 3;
                        break;
                case 28800:
                        bsel   = 137;
                        bscale = -1;
                        break;
                case 38400:
                        bsel   = 12;
                        bscale = 2;
                        break;
                case 57600:
                        bsel   = 135;
                        bscale = -2;
                        break;
                case 76800:
                        bsel   = 12;
                        bscale = 1;
                        break;
                case 115200:
                        bsel   = 131;
                        bscale = -3;
                        break;
                case 230400:
                        bsel   = 123;
                        bscale = -4;
                        break;
                case 460800:
                        bsel   = 107;
                        bscale = -5;
                        break;
		default:
			return;
	}

        USART_Format_Set(&USART, csize, pmode, twosb);
        USART_Baudrate_Set(&USART, bsel, bscale);

        USART_Rx_Enable(&USART);
        USART_Tx_Enable(&USART);

	configured = 1;
}

ISR(USARTE0_RXC_vect) {
        USART_RXComplete(&USART_data);
}

