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


/* Hardware library includes. */
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"

/* driverlib library includes */
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

#include "driverlib/uart.h"

#include "driverlib/debug.h"

#include "utils/vstdlib.h"
#include "modeeprom.h"

static const char * const g_pcHex = "0123456789abcdef";

const char * const welcome = "\r\nnux modeeprom test V1.0";

static void prvSetupHardware( void ); // configure the hardware

extern void uart_init(unsigned short uart_idx, unsigned long baud, unsigned short config);
/*-----------------------------------------------------------*/

void INFO(const char *pcString, ...);

extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);
extern int UARTgets(unsigned long ulBase, char *pcBuf, unsigned long ulLen);

void blinky(unsigned int count) {
volatile unsigned long ulLoop;
    while( 0 < count-- )    {
        //
        for(ulLoop = 0; ulLoop < 2000000; ulLoop++);
        //
        // Output high level
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
        //
        // Delay some time
        //
        for(ulLoop = 0; ulLoop < 2000000; ulLoop++);
        //
        // Output low level
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ~GPIO_PIN_0);
    }
}

int test_init() {

	MODEE_init();
	INFO("test_init  ..... PASS");

	return 1;
}

int test_module_exists() {
	short int result;
	
	result = MODEE_exists(SLAVE_ADDRESS_MODULE1);
	if( result == 1) {
		INFO("test_module_exists .... PASS");
	} else {
		INFO("test_module_exists .... FAIL");
	}
	
	return result;
}

int test_write_read() {
	unsigned char buf[20];
	unsigned char inbuf[20];
	
	buf[0]='1';
	buf[1]='2';
	buf[2]='3';
	buf[3]='4';
	buf[4]=0;
	usnprintf((char *)&inbuf,20,"xxxxxxxxxxxxxxxxxxxxxxxxxxx");

	MODEE_write(SLAVE_ADDRESS_MODULE1,(unsigned char *) &buf, 5, 0);
	
	MODEE_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) &inbuf, 5, 0);
	if(ustrncmp((const char *)&buf,(const char *)&inbuf,5) == 0)	{
		INFO("test_write_read .... PASS");
	} else {
		INFO("test_write_read .... FAIL");
		INFO("data: %s",&inbuf);
	}

	return 1;
}


int test_write_read_long() {
	unsigned char buf[20];
	unsigned char inbuf[20];
	
	usnprintf((char *)&buf,20,"Long String with >16 char.");
	usnprintf((char *)&inbuf,20,"xxxxxxxxxxxxxxxxxxxxxxxxxxx");

	MODEE_write(SLAVE_ADDRESS_MODULE1,(unsigned char *) &buf, 20, 0);
	
	MODEE_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) &inbuf, 20, 0);
	if(ustrncmp((const char *)&buf,(const char *)&inbuf,5) == 0)	{
		INFO("test_write_read_long .... PASS");
	} else {
		INFO("test_write_read_long .... FAIL");
		INFO("data: %s",&inbuf);
	}

	return 1;
}


int main( void ) {
	char cmd_buf[10];
	
	prvSetupHardware();
	

	for( ;; ) {
		blinky(2);
		INFO(welcome);
		UARTgets(UART0_BASE,cmd_buf, 10);
		test_init();
		test_module_exists();
		test_write_read();
		test_write_read_long();
	}
	return 0;
}
/*-----------------------------------------------------------*/

void prvSetupHardware( void ){
		
    /* If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    a workaround to allow the PLL to operate reliably. */
    if( DEVICE_IS_REVA2 )    {
        SysCtlLDOSet( SYSCTL_LDO_2_75V );
    }

	/* Set the clocking to run from the PLL at 50 MHz */
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );

    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Enable the GPIO pin for the LED (PF0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

	uart_init(0, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

}
/*-----------------------------------------------------------*/


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
void INFO(const char *pcString, ...) {
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
	
	UARTSend(UART0_BASE, "\r\n", 2);
	va_end(vaArgP);
}


