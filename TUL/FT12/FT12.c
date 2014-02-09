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

#include "FT12.h"

#ifdef NO_PGM
#undef DEBUG
#endif

#ifdef DEBUG
#include <stdio.h>
#include <avr/pgmspace.h>
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...)
#endif

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t toTPUART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      toTPUART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t toUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      toUSB_Buffer_Data[128];

static uint8_t      inFT12_Buffer[256];
static uint8_t      inFT12_count;
static uint8_t      inTPU_Buffer[65];
static uint8_t      inTPU_count;
static uint8_t      toggles = 0;
static uint8_t      E5next = 0;
static uint8_t      waitTPUconfirm   = 0;
static uint8_t      waitTPUreceiving = 0;
static uint8_t      data[65];
static uint8_t      confirmdata[65];
static uint8_t      ETSmode = 0;

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
#ifdef DEBUG
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

int usb_putchar (char d, FILE * stream) {
        CDC_Device_SendByte(&VirtualSerial2_CDC_Interface, d);
        return 0;
}

#endif

void FT12_answer( uint8_t *d, uint8_t count, uint8_t len ) {
	uint8_t crc = 0xff;
	RingBuffer_Insert(&toUSB_Buffer, 0x68);
	if (len) {
		RingBuffer_Insert(&toUSB_Buffer, len);
		RingBuffer_Insert(&toUSB_Buffer, len);
	} else {
		RingBuffer_Insert(&toUSB_Buffer, count+1);
		RingBuffer_Insert(&toUSB_Buffer, count+1);
	}
	RingBuffer_Insert(&toUSB_Buffer, 0x68);

	if (toggles & 1) crc=0xD3; else crc=0xF3; 
	toggles ^= 1;
	RingBuffer_Insert(&toUSB_Buffer, crc);

	for (uint8_t i=0;i<count;i++) {
	  	RingBuffer_Insert(&toUSB_Buffer, d[i]);
		crc += d[i];
		if (ETSmode && !len && (d[i]==1))
	  	  RingBuffer_Insert(&toUSB_Buffer, 0);

	}

	RingBuffer_Insert(&toUSB_Buffer, crc);
	RingBuffer_Insert(&toUSB_Buffer, 0x16);
}

// process FT12 message
void process_FT12(void) {
        uint8_t i;

	memset( data, 0, sizeof(data));

	// second part handling
	if ((inFT12_Buffer[0] == 0xE5)) {
		switch (E5next) {
			case 1:
				memcpy_P( data, PSTR("\x89\x00\x00\x00\x00\x00\x66\x03\xD6\x01\x05\x10\x01\x01"), 14);
				FT12_answer( data, 14, 0 ); 
				break;
			case 2:
				memcpy_P( data, PSTR("\x89\x00\x00\x00\x00\x00\x63\x03\x40\x00\x21"), 11);
				FT12_answer( data, 11, 0 ); 
				break;
			case 3:
				memcpy_P( data, PSTR("\x89\x0C\x00\x00\x00\x00\x04\x02\x41\x01\x16\x00"), 12);
				FT12_answer( data, 12, 0 ); 
				break;
			case 4:
				memcpy_P( data, PSTR("\x89\x00\x00\x00\x00\x00\x66\x03\xD6\x01\x05\x10\x01\x02"), 14);
				FT12_answer( data, 14, 0 ); 
				break;
		}
		E5next = 0;
		return;
	}

	E5next = 0;

	uint8_t mlen = inFT12_Buffer[1];
	switch (inFT12_Buffer[5]) {
		case 0x11: // L_Data.req

			inFT12_Buffer[6] |= 0xB0;
			inFT12_Buffer[7] = 0x11;
			inFT12_Buffer[8] = 0xFF;

			PRINTF( "mew message to TPUART\r\n");
 			uint8_t crc = 0xff;
			for (i=0; i<(mlen-2); i++) {
//				RingBuffer_Insert(&toTPUART_Buffer, 0x80 | i);
//				RingBuffer_Insert(&toTPUART_Buffer, inFT12_Buffer[6+i]);

               			Serial_SendByte(0x80 | i);
               			Serial_SendByte(inFT12_Buffer[6+i]);
				crc ^= inFT12_Buffer[6+i];
			}

//			RingBuffer_Insert(&toTPUART_Buffer, 0x40 | i);
//			RingBuffer_Insert(&toTPUART_Buffer, crc);
               		Serial_SendByte(0x40 | i);
               		Serial_SendByte(crc);

			RingBuffer_Insert(&toUSB_Buffer, 0xE5);

			waitTPUconfirm = mlen-2;
			memset( confirmdata, 0, sizeof( confirmdata ));
			confirmdata[0] = 0x2E;
			memcpy( &confirmdata[1], &inFT12_Buffer[6], waitTPUconfirm );

			break;
		case 0x43: // T_Connect.req 
			ETSmode = 1;
			memcpy_P( data, PSTR("\x86\x00\x00\x00\x00\x00"), 6);
			FT12_answer( data, 6, 0 ); 
			break;
		case 0x41: // T_Data_Connected.req
			ETSmode = 1;
			if (memcmp_P(&inFT12_Buffer[6], PSTR("\x00\x00\x00\x00\x00\x65\x03\xD5\x01\x05\x10\x01"), 12 )==0) {
				memcpy_P( data, PSTR("\x8E\x00\x00\x00\x00\x00\x65\x03\xD5\x01\x05\x10\x01"), 13);
				FT12_answer( data, 13, 0 ); // -2 
				E5next = 1;
			}
			else if (memcmp_P(&inFT12_Buffer[6], PSTR("\x00\x00\x00\x00\x00\x61\x03\x00"), 8 )==0) {
				memcpy_P( data, PSTR("\x8E\x00\x00\x00\x00\x00\x61\x03\x00"), 9);
				FT12_answer( data, 9, 0 ); // correct 
				E5next = 2;
			}
			else if (memcmp_P(&inFT12_Buffer[6], PSTR("\x0C\x00\x00\x00\x00\x03\x02\x01\x01\x16"), 10 )==0) {
				memcpy_P( data, PSTR("\x8E\x0C\x00\x00\x00\x00\x03\x02\x01\x01\x16"), 11);
				FT12_answer( data, 11, 0 ); // -2 
				E5next = 3;
			}
			else if (memcmp_P(&inFT12_Buffer[6], PSTR("\x00\x00\x00\x00\x00\x6F\x03\xD7\x01\x05\x10\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"), 22 )==0) {
				memcpy_P( data, PSTR("\x8E\x00\x00\x00\x00\x00\x6F\x03\xD7\x01\x05\x10\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"), 23);
				FT12_answer( data, 23, 0 ); // -3 
				E5next = 4;
			}
			else if (memcmp_P(&inFT12_Buffer[6], PSTR("\x0C\x00\x00\x00\x00\x04\x02\x81\x01\x16"), 10 )==0) {
				memcpy_P( data, PSTR("\x8E\x0C\x00\x00\x00\x00\x04\x02\x81\x01\x16\x00"), 12);
				FT12_answer( data, 12, 0 ); // -1 
			}
			else {
				PRINTF( "No match for 0x41-Request\r\n" );
			}
			break;
		case 0x44: 
			ETSmode = 1;
			memcpy_P( data, PSTR("\x88\x00\x00\x00\x00\x00"), 6);
			FT12_answer( data, 6, 0 ); 
			break;
		case 0xA7: // PEI_Identify.req
			ETSmode = 1;
			memcpy_P( data, PSTR("\xA8\x11\xFF\x00\x01\x00\x00\xE4\x5A"), 9);
			FT12_answer( data, 9, 0 ); 
			break;
	}
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void) {
	SetupHardware();

     	RingBuffer_InitBuffer(&toTPUART_Buffer, toTPUART_Buffer_Data, sizeof(toTPUART_Buffer_Data));
     	RingBuffer_InitBuffer(&toUSB_Buffer, toUSB_Buffer_Data, sizeof(toUSB_Buffer_Data));

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	for (;;) {

		/*
		*********** incoming data processing **********
		*/

#ifdef DEBUG
		/* Discard all received data on the first CDC interface */
		CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
#endif

		/*
		*********** raw incoming TPUART message processing **********
		*/
		int16_t TPUByte = Serial_ReceiveByte();
		if (!(TPUByte < 0)) {
     			PRINTF( "from TPUART: %02X\r\n", TPUByte );
			inTPU_Buffer[inTPU_count++] = TPUByte;

		  	switch(inTPU_Buffer[0]) {
				case 0x9C:
				case 0x90:
				case 0x98:
				case 0x94:
				case 0xBC:
				case 0xB0:
				case 0xB8:
				case 0xB4:
					waitTPUreceiving = 1;
					if (inTPU_count==5)
							if((inTPU_Buffer[3]==0x11) && (inTPU_Buffer[4]==0xFF)) {
								Serial_SendByte(0x11);
								PRINTF( "to TPUART: %02X\r\n", 0x11);
							}
								
					if (inTPU_count>=8) {
						uint8_t elen = (inTPU_Buffer[5]&0xf)+8;
						if (inTPU_count>=elen) {
							PRINTF( "EIB message complete length: %d\r\n", elen );

							memset( data, 0, sizeof(data));
							data[0] = 0x29;
							for (uint8_t i=0;i<elen;i++)
								data[i+1] = inTPU_Buffer[i];

							if (waitTPUconfirm) {
								waitTPUconfirm = elen-1;
								memcpy( &confirmdata[1], inTPU_Buffer, waitTPUconfirm );
							} else {

								FT12_answer( data, elen, 0 ); 
							}

							inTPU_count -= elen;
							memcpy( inTPU_Buffer, &inTPU_Buffer[elen], inTPU_count);
							waitTPUreceiving = 0;
						}
					}
					break;

				case 0x8B:
					PRINTF( "Confirming frame sent ...\r\n" );
					if (waitTPUconfirm) 
						FT12_answer( confirmdata, waitTPUconfirm+1, 0);

				case 0x0B:
					waitTPUconfirm   = 0;
					waitTPUreceiving = 0;

					inTPU_count -= 1;
					memcpy( inTPU_Buffer, &inTPU_Buffer[1], inTPU_count);

					break;

				default:
					waitTPUreceiving = 0;
					PRINTF( "unhandled TPUART code: %02X\r\n", inTPU_Buffer[0] );
					inTPU_count -= 1;
					memcpy( inTPU_Buffer, &inTPU_Buffer[1], inTPU_count);
			}	
			
		}	
	
		/*
		*********** raw incoming FT12 message processing **********
		*/

		int16_t FT12Byte = CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);
		while (!(FT12Byte < 0)) {
			inFT12_Buffer[inFT12_count++] = FT12Byte;
			PRINTF( "%02X ", FT12Byte );
			FT12Byte = CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);	
		}

		// find sync
		while (inFT12_count) {
			if ((inFT12_Buffer[0] != 0x68) && (inFT12_Buffer[0] != 0x10)) {
				// some confirm?
				if (inFT12_Buffer[0] == 0xE5) {
					process_FT12();
				}
				inFT12_count -= 1;
				memcpy( inFT12_Buffer, &inFT12_Buffer[1], inFT12_count);
				continue;
			}
			break;
		}

		if (inFT12_count>=4) {
		   	if ( memcmp_P( inFT12_Buffer, PSTR("\x10\x40\x40\x16"), 4) == 0) {
				PRINTF( "Received FT12 reset\r\n" );

				// confirm it
				RingBuffer_Insert(&toUSB_Buffer, 0xE5);
				
				waitTPUconfirm   = 0;
				waitTPUreceiving = 0;
				inFT12_count     = 0;
				inTPU_count      = 0;
				ETSmode          = 0;

				RingBuffer_Insert(&toTPUART_Buffer, 0x01);

		   	} else if ( memcmp_P( inFT12_Buffer, PSTR("\x10\x49\x49\x16"), 4) == 0) {

				RingBuffer_Insert(&toUSB_Buffer, 0x10);
				RingBuffer_Insert(&toUSB_Buffer, 0x8B);
				RingBuffer_Insert(&toUSB_Buffer, 0x8B);
				RingBuffer_Insert(&toUSB_Buffer, 0x16);

				inFT12_count -= 4;
				memcpy( inFT12_Buffer, &inFT12_Buffer[4], inFT12_count);

			} else if ((inFT12_Buffer[0] == 0x68) && (inFT12_Buffer[3] == 0x68) && (inFT12_Buffer[1] == inFT12_Buffer[2])) {
				uint8_t mlen = inFT12_Buffer[1]+6;
	
				if ((mlen <= inFT12_count)) {
					if (inFT12_Buffer[mlen-1] == 0x16 ) {

						if ((inFT12_Buffer[5] == 0x11) && (waitTPUreceiving || waitTPUconfirm)) {
							// last sent to TPU pending ...
						} else {
							PRINTF( "Received FT12 message len: %d\r\n", mlen );

							// confirm it
							if (inFT12_Buffer[5] != 0x11)
								RingBuffer_Insert(&toUSB_Buffer, 0xE5);
					
							process_FT12();

							inFT12_count -= mlen;
							memcpy( inFT12_Buffer, &inFT12_Buffer[mlen], inFT12_count);
						}

					} else {
						PRINTF( "Received broken FT12 message len: %d - EOM: %d\r\n", mlen, inFT12_Buffer[mlen-1] );
						// remove the valid head ...
						inFT12_count -= 4;
						memcpy( inFT12_Buffer, &inFT12_Buffer[4], inFT12_count);
					}
				}
				
			} else {
				// unknown message head
				inFT12_count -= 4;
				memcpy( inFT12_Buffer, &inFT12_Buffer[4], inFT12_count);

			}
		}


		/*
		*********** outgoing Ringbuffer processing **********
		*/

          	/* process toUSB path */
          	uint16_t BufferCount = RingBuffer_GetCount(&toUSB_Buffer);
          	if (BufferCount) {

               		LEDs_SetAllLEDs( LEDS_NO_LEDS );

               		/* Read bytes from the buffer into the USB IN endpoint */
               		while (BufferCount--) {
                    		/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
                    		if (CDC_Device_SendByte(&VirtualSerial1_CDC_Interface, RingBuffer_Peek(&toUSB_Buffer)) != ENDPOINT_READYWAIT_NoError) {
                         		break;
                    		}

                    		/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
				uint8_t byte = RingBuffer_Remove(&toUSB_Buffer);
                    		PRINTF( "%02X ", byte);
               		}
			PRINTF( "- send to FT12 host\r\n" );
          	}


          	/* Load the next byte from the TPUART transmit buffer into the USART */
		if (!(RingBuffer_IsEmpty(&toTPUART_Buffer)))
          		if (UCSR1A & (1 << UDRE1)) {
				uint8_t byte = RingBuffer_Remove(&toTPUART_Buffer);
               			Serial_SendByte(byte);
				PRINTF( "to TPUART: %02X\r\n", byte);
          		}

		/*
		*********** USB processing **********
		*/

		CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);
#ifdef DEBUG
		CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
#endif
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

#ifdef DEBUG
        fdevopen(&usb_putchar,NULL);
#endif

	Serial_Init(19200, true);
	UCSR1C |= (1 << UPM11);
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
#ifdef DEBUG
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial2_CDC_Interface);
#endif

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial1_CDC_Interface);
#ifdef DEBUG
	CDC_Device_ProcessControlRequest(&VirtualSerial2_CDC_Interface);
#endif
}


void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) {
	if (CDCInterfaceInfo->Config.ControlInterfaceNumber) 
		return;

        PRINTF( "ControlLines: %02X\r\n", CDCInterfaceInfo->State.ControlLineStates.HostToDevice );

	// pretend we are always "online"
	CDCInterfaceInfo->State.ControlLineStates.DeviceToHost |= CDC_CONTROL_LINE_IN_DCD;
	CDCInterfaceInfo->State.ControlLineStates.DeviceToHost |= CDC_CONTROL_LINE_IN_DSR;
	CDC_Device_SendControlLineStateChange(CDCInterfaceInfo);
}

