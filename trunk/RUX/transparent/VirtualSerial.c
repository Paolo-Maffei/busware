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
#include "usart_driver.h"

#define USART USARTC0
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

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs
 */
static FILE USBSerialStream;

int usb_putchar (char data, FILE * stream) {
        fputc(data, &USBSerialStream);
        return 0;
}

uint16_t loop;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	/* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

#ifdef BOARD_HAS_LEDS
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
#endif
	GlobalInterruptEnable();

	for (;;)
	{

		/* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
                int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (!(ReceivedByte < 0)) {
			//printf( "%02x ", ReceivedByte & 0xff);
			switch (ReceivedByte & 0xff) {
				case '1':
// 01 21 20 29 3F AA
// 01 21 10 29 3F 9A
// 01 21 11 29 3F 9B 9B
// "\x01\x2F\x20\x29\x01\x7A\x01\x2F\x10\x29\x01\x6A\x01\x2F\x11\x29\x01\x6B\x6B"
					USART_TXBuffer_PutByte(&USART_data, 0x01);
					USART_TXBuffer_PutByte(&USART_data, 0x2f);
					USART_TXBuffer_PutByte(&USART_data, 0x20);
					USART_TXBuffer_PutByte(&USART_data, 0x29);
					USART_TXBuffer_PutByte(&USART_data, 0x01);
					USART_TXBuffer_PutByte(&USART_data, 0x7a);

					USART_TXBuffer_PutByte(&USART_data, 0x01);
					USART_TXBuffer_PutByte(&USART_data, 0x2f);
					USART_TXBuffer_PutByte(&USART_data, 0x10);
					USART_TXBuffer_PutByte(&USART_data, 0x29);
					USART_TXBuffer_PutByte(&USART_data, 0x01);
					USART_TXBuffer_PutByte(&USART_data, 0x6a);

					USART_TXBuffer_PutByte(&USART_data, 0x01);
					USART_TXBuffer_PutByte(&USART_data, 0x2f);
					USART_TXBuffer_PutByte(&USART_data, 0x11);
					USART_TXBuffer_PutByte(&USART_data, 0x29);
					USART_TXBuffer_PutByte(&USART_data, 0x01);
					USART_TXBuffer_PutByte(&USART_data, 0x6b);
				//	USART_TXBuffer_PutByte(&USART_data, 0x6b);

					break;
			}
		}

                while (USART_RXBufferData_Available(&USART_data)) {
			PORTD.OUTTGL = PIN5_bm;
			uint8_t b = USART_RXBuffer_GetByte(&USART_data);
			printf( "%02X ", b);
                }

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();

		if (loop++) continue;

		PORTD.OUTTGL = PIN5_bm;
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;

	// LED
	PORTD.DIRSET = PIN5_bm;

	// DIR
	PORTC.DIRSET = PIN1_bm;

	// LED ON
	PORTD.OUTSET = PIN5_bm;

	// Receiving
	PORTC.OUTCLR = PIN1_bm;

#endif

	/* Hardware Initialization */
#ifdef BOARD_HAS_LEDS
	LEDs_Init();
#endif

	// USART RX/TX 1
	/* PIN3 (TXD0) as output. */
        PORTC.DIRSET = PIN3_bm;
        /* PC2 (RXD0) as input. */
        PORTC.DIRCLR = PIN2_bm;

        /* Use USARTF0 and initialize buffers. */
        USART_InterruptDriver_Initialize(&USART_data, &USART, USART_DREINTLVL_LO_gc);

        /* USARTC0, 8 Data bits, No Parity, 1 Stop bit. */
        USART_Format_Set(USART_data.usart, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc, false);

        /* Enable RXC interrupt. */
        USART_RxdInterruptLevel_Set(USART_data.usart, USART_RXCINTLVL_LO_gc);

        /* Set Baudrate to 38400 bps:
         * Use the default I/O clock frequency that is 32 MHz.
         * Do not use the baudrate scale factor
         *
         * Baudrate select = (1/(16*(((I/O clock frequency)/Baudrate)-1)
         *                 = 12
         */
        USART_Baudrate_Set(&USART, 12 , 4); // 9600 
//        USART_Baudrate_Set(&USART, 24 , 0); // 19200

        /* Enable both RX and TX. */
        USART_Rx_Enable(USART_data.usart);
        USART_Tx_Enable(USART_data.usart);

        /* Enable global interrupts. */
        sei();

	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
#ifdef BOARD_HAS_LEDS
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
#endif
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
#ifdef BOARD_HAS_LEDS
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
#endif
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

#ifdef BOARD_HAS_LEDS
        LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
#endif
        fdevopen(&usb_putchar,NULL);

}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/*! \brief Receive complete interrupt service routine.
 *
 *  Receive complete interrupt service routine.
 *  Calls the common receive complete handler with pointer to the correct USART
 *  as argument.
 */
ISR(USARTC0_RXC_vect)
{
        USART_RXComplete(&USART_data);
}


/*! \brief Data register empty  interrupt service routine.
 *
 *  Data register empty  interrupt service routine.
 *  Calls the common data register empty complete handler with pointer to the
 *  correct USART as argument.
 */
ISR(USARTC0_DRE_vect)
{
        USART_DataRegEmpty(&USART_data);
}

