/*************************************************************************
Copyright (C) 2011  busware

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*************************************************************************/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Hardware library includes. */
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ints.h"
#include "inc/hw_watchdog.h"

/* driverlib library includes */
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/grlib.h"
#include "driverlib/uart.h"
#include "driverlib/watchdog.h"
#include "driverlib/debug.h"

#include "lwip/tcpip.h"
#include "lwiplib.h"
#include "lwip/netif.h"

#include "utils/vstdlib.h"

#include "LWIPStack.h"
#include "ETHIsr.h"
#include "softeeprom.h"
#include "console.h"

/*-----------------------------------------------------------*/
static const char * const g_pcHex = "0123456789abcdef";


#define mainBASIC_TELNET_STACK_SIZE            ( configMINIMAL_STACK_SIZE * 2 )


#define SIM_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE + 100)

/* Task priorities. */
#define CHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )

#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))

/*-----------------------------------------------------------*/
void console( void *pvParameters );
static void prvSetupHardware( void ); // configure the hardware

/*-----------------------------------------------------------*/

volatile unsigned short should_reset; // watchdog variable to perform a reboot

// global stats
unsigned int stats_queue_full;
unsigned int stats_uart1_rcv;
unsigned int stats_uart1_sent;



/*
  required when compiling with MemMang/heap_3.c
*/
extern int  __HEAP_START;

extern void *_sbrk(int incr) {
    static unsigned char *heap = NULL;
    unsigned char *prev_heap;

    if (heap == NULL) {
        heap = (unsigned char *)&__HEAP_START;
    }
    prev_heap = heap;

    heap += incr;

    return (void *)prev_heap;
}

extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);

void blinky(unsigned int count) {

    while( 0 < count-- )    {
        //
        // Turn on the LED.
        //
        GPIO_PORTF_DATA_R |= 0x01;
		vTaskDelay(1000 / portTICK_RATE_MS);

        //
        // Turn off the LED.
        //
        GPIO_PORTF_DATA_R &= ~(0x01);

		vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void ethernetThread(void *pvParameters) {
	IP_CONFIG ipconfig;
	tBoolean found, found_addr;
    long lEEPROMRetStatus;
    unsigned short usdata,usdata2;

	ETHServiceTaskInit(0);
	ETHServiceTaskFlush(0,ETH_FLUSH_RX | ETH_FLUSH_TX);

    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPADDR_HIGH_ID, &usdata2, &found_addr);
    lEEPROMRetStatus = SoftEEPROMRead(IPMODE_ID, &usdata, &found);
	if(lEEPROMRetStatus == 0 && found && found_addr) {
		ipconfig.IPMode = usdata;
	} else {
		ipconfig.IPMode = IPADDR_USE_DHCP;
	}

	// read static ip address from eeprom
	if(ipconfig.IPMode == IPADDR_USE_STATIC) {
		SoftEEPROMRead(STATIC_IPADDR_LOW_ID, &usdata, &found);
		ipconfig.IPAddr = (usdata2 << 16 & 0xFFFF0000) | (usdata & 0x0000FFFF) ;
		
	    SoftEEPROMRead(STATIC_IPMASK_HIGH_ID, &usdata2, &found);
	    SoftEEPROMRead(STATIC_IPMASK_LOW_ID, &usdata, &found);
		ipconfig.NetMask = (usdata2 << 16 & 0xFFFF0000) | (usdata & 0x0000FFFF) ;
		
	    SoftEEPROMRead(STATIC_IPGW_HIGH_ID, &usdata2, &found);
	    SoftEEPROMRead(STATIC_IPGW_LOW_ID, &usdata, &found);
		ipconfig.GWAddr = (usdata2 << 16 & 0xFFFF0000) | (usdata & 0x0000FFFF) ;
	}

	LWIPServiceTaskInit((void *)&ipconfig);

	// Nothing else to do.  No point hanging around.
	vTaskDelete( NULL);
}
/*************************************************************************
 * Please ensure to read http://www.freertos.org/portLM3Sxxxx_Eclipse.html
 * which provides information on configuring and running this demo for the
 * various Luminary Micro EKs.
 *************************************************************************/
int main( void ) {
	stats_queue_full=0;
	stats_uart1_rcv =0;
	stats_uart1_sent =0;
	
	prvSetupHardware();

	/* Create the uIP task if running on a processor that includes a MAC and
	PHY. */
	if( SysCtlPeripheralPresent( SYSCTL_PERIPH_ETH ) )	{
		xTaskCreate( ethernetThread, ( signed portCHAR * ) "uIP", mainBASIC_TELNET_STACK_SIZE, NULL, CHECK_TASK_PRIORITY , NULL );
	}

	if (pdPASS != xTaskCreate( console, ( signed portCHAR * ) "CONS", SIM_TASK_STACK_SIZE, NULL, CHECK_TASK_PRIORITY , NULL )) {
		LWIPDebug("Cant create console!");
	}

	vTaskStartScheduler(); // Start the scheduler. 

    /* Will only get here if there was insufficient memory to create the idle
    task. */
	for( ;; );
	return 0;
}
/*-----------------------------------------------------------*/

void prvSetupHardware( void ){
	tBoolean found;
    long lEEPROMRetStatus;
    unsigned short data,data2;
	unsigned long uart_speed;
	
    /* If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    a workaround to allow the PLL to operate reliably. */
    if( DEVICE_IS_REVA2 )    {
        SysCtlLDOSet( SYSCTL_LDO_2_75V );
    }

	/* Set the clocking to run from the PLL at 50 MHz */
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );

	/* 	Enable Port F for Ethernet LEDs
		LED0        Bit 3   Output
		LED1        Bit 2   Output */
	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOF );
	GPIODirModeSet( GPIO_PORTF_BASE, (GPIO_PIN_2 | GPIO_PIN_3), GPIO_DIR_MODE_HW );
	GPIOPadConfigSet( GPIO_PORTF_BASE, (GPIO_PIN_2 | GPIO_PIN_3 ), GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD );


    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;

    //
    // Enable the GPIO pin for the LED (PF0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIO_PORTF_DIR_R = 0x01;
    GPIO_PORTF_DEN_R = 0x01;

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	
    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Check to see if an error occurred.
    //
    if(SoftEEPROMInit(0x1F000, 0x20000, 0x800) != 0)  {
		LWIPDebug("SoftEEPROM initialisation failed.");
    }


    lEEPROMRetStatus = SoftEEPROMRead(UART0_SPEED_HIGH_ID, &data, &found);
	if(lEEPROMRetStatus == 0 && found) {
	    SoftEEPROMRead(UART0_SPEED_LOW_ID, &data2, &found);
		uart_speed = (data << 16 & 0xFFFF0000) | (data2 & 0x0000FFFF);
	    SoftEEPROMRead(UART0_CONFIG_ID, &data, &found);
	} else {
		uart_speed=115200;
		data = (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	}
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), uart_speed,  data);


    lEEPROMRetStatus = SoftEEPROMRead(UART1_SPEED_HIGH_ID, &data, &found);
	if(lEEPROMRetStatus == 0 && found) {
	    SoftEEPROMRead(UART1_SPEED_LOW_ID, &data2, &found);
		uart_speed = (data << 16 & 0xFFFF0000) | (data2 & 0x0000FFFF);
	    SoftEEPROMRead(UART1_CONFIG_ID, &data, &found);
	} else {
		uart_speed=38400;
		data = (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	}
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), uart_speed, data);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    IntEnable(INT_WATCHDOG); // Enable the watchdog interrupt.
    WatchdogReloadSet(WATCHDOG0_BASE, SysCtlClockGet());
    WatchdogResetEnable(WATCHDOG0_BASE);
    WatchdogEnable(WATCHDOG0_BASE);
}
/*-----------------------------------------------------------*/


void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName ) {
	LWIPDebug("Stackoverflow task:%s",pcTaskName);
}

/* This function can't fragment the memory. The message gets parsed, formatted and print to UART0.

 Only the following formatting characters are supported:
 - \%c to print a character
 - \%d to print a decimal value
 - \%s to print a string
  - \%u to print an unsigned decimal value
  - \%x to print a hexadecimal value using lower case letters
  - \%X to print a hexadecimal value using lower case letters (not upper case
  letters as would typically be used)
  - \%p to print a pointer as a hexadecimal value
  - \%\% to print out a \% character
 
  For \%s, \%d, \%u, \%p, \%x, and \%X, an optional number may reside
  between the \% and the format character, which specifies the minimum number
  of characters to use for that value; if preceded by a 0 then the extra
  characters will be filled with zeros instead of spaces.  For example,
  ``\%8d'' will use eight characters to print the decimal value with spaces
  added to reach eight; ``\%08d'' will use eight characters as well but will
  add zeroes instead of spaces.
 
  The type of the arguments after \e pcString must match the requirements of
  the format string.  For example, if an integer was passed where a string
  was expected, an error of some kind will most likely occur.
 
*/
void LWIPDebug(const char *pcString, ...) {
	unsigned long ulIdx, ulValue, ulPos, ulCount, ulBase, ulNeg;
	char *pcStr, pcBuf[16], cFill;
	va_list vaArgP;
	ASSERT(pcString != 0);

	va_start(vaArgP, pcString);
	while(*pcString) {
		// Find the first non-% character, or the end of the string.
		for(ulIdx = 0; (pcString[ulIdx] != '%') && (pcString[ulIdx] != '\0'); ulIdx++) { }

		UARTSend(UART0_BASE, pcString, ulIdx);
		pcString += ulIdx;

		if(*pcString == '%') { // See if the next character is a %.
			pcString++;

			ulCount = 0;
			cFill = ' ';

			// It may be necessary to get back here to process more characters.
			// Goto's aren't pretty, but effective.  I feel extremely dirty for
			// using not one but two of the beasts.
			again:

			switch(*pcString++) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9': {
					// If this is a zero, and it is the first digit, then the
					// fill character is a zero instead of a space.
					if((pcString[-1] == '0') && (ulCount == 0)) {
						cFill = '0';
					}

					ulCount *= 10;
					ulCount += pcString[-1] - '0';

					goto again;
				}

				case 'c': {
					ulValue = va_arg(vaArgP, unsigned long);
					UARTSend(UART0_BASE,(char *)&ulValue, 1);
					break;
				}

				case 'd': {
					ulValue = va_arg(vaArgP, unsigned long);
					ulPos = 0;

					// If the value is negative, make it positive and indicate
					// that a minus sign is needed.
					if((long)ulValue < 0) {
						ulValue = -(long)ulValue;
						ulNeg = 1;
					} else {
						ulNeg = 0;
					}
					ulBase = 10;
					goto convert;
				}
				case 's': {
					pcStr = va_arg(vaArgP, char *);
					for(ulIdx = 0; pcStr[ulIdx] != '\0'; ulIdx++) {}

					UARTSend(UART0_BASE,pcStr, ulIdx);

					if(ulCount > ulIdx) {
						ulCount -= ulIdx;
						while(ulCount--) {
							UARTSend(UART0_BASE," ", 1);
						}
					}
					break;
				}
				case 'u': {
					ulValue = va_arg(vaArgP, unsigned long);
					ulPos = 0;
					ulBase = 10;
					ulNeg = 0;
					goto convert;
				}
				case 'x':
				case 'X':
				case 'p': {
					ulValue = va_arg(vaArgP, unsigned long);
					ulPos = 0;
					ulBase = 16;
					ulNeg = 0;
					convert:
					for(ulIdx = 1;
					(((ulIdx * ulBase) <= ulValue) &&
						(((ulIdx * ulBase) / ulBase) == ulIdx));
					ulIdx *= ulBase, ulCount--) {}

					// If the value is negative, reduce the count of padding
					// characters needed.
					if(ulNeg) {
						ulCount--;
					}

					// If the value is negative and the value is padded with
					// zeros, then place the minus sign before the padding.
					if(ulNeg && (cFill == '0')) {
						pcBuf[ulPos++] = '-';
						ulNeg = 0;
					}

					// Provide additional padding at the beginning of the
					// string conversion if needed.
					if((ulCount > 1) && (ulCount < 16)) {
						for(ulCount--; ulCount; ulCount--) {
							pcBuf[ulPos++] = cFill;
						}
					}

					if(ulNeg) {
						pcBuf[ulPos++] = '-';
					}

					// Convert the value into a string.
					for(; ulIdx; ulIdx /= ulBase) {
						pcBuf[ulPos++] = g_pcHex[(ulValue / ulIdx) % ulBase];
					}

					UARTSend(UART0_BASE,pcBuf, ulPos);
					break;
				}
				case '%': {
					UARTSend(UART0_BASE,pcString - 1, 1);
					break;
				}
				default: {
					UARTSend(UART0_BASE,"ERROR", 5);
					break;
				}
			}
		}
	}
	va_end(vaArgP);
}

/*
	Watchdog handler. 
*/
void WatchdogIntHandler(void) {
    if (! should_reset) {
	    WatchdogIntClear(WATCHDOG0_BASE);
	}

}
