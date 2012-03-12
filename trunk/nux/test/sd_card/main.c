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

/* driverlib library includes */
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/grlib.h"
#include "driverlib/uart.h"

#include "driverlib/debug.h"
#include "utils/cmdline.h"
#include "utils/vstdlib.h"

#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"

#define TELNETD_CONF_LINELEN (120)
#define SIM_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE + 100)
#define CHECK_TASK_PRIORITY			( tskIDLE_PRIORITY + 3 )

static const char * const g_pcHex = "0123456789abcdef";

/*-----------------------------------------------------------*/
static void prvSetupHardware( void ); // configure the hardware
extern void uart_init(unsigned short uart_idx, unsigned long baud, unsigned short config);
/*-----------------------------------------------------------*/

volatile unsigned short should_reset; // watchdog variable to perform a reboot

// global stats

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
extern int UARTgets(unsigned long ulBase, char *pcBuf, unsigned long ulLen);
void INFO(const char *pcString, ...);


/*****************************************************************************
! The buffer that holds the command line entry.
*****************************************************************************/

const portCHAR * const welcome = "\r\nSD card console V1.0\r\nType \'help\' for help.\r\n";
const portCHAR * const prompt = "\r\n> ";

/* The time between cycles of the 'check' functionality (defined within the
tick hook. */
#define mainCHECK_DELAY						( ( portTickType ) 10 / portTICK_RATE_MS )

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary
// data from the SD card.  There are two buffers allocated of this size.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
//
//*****************************************************************************
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_cTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code,
// and a string represenation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT fresult;
    char *pcResultStr;
}
tFresultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and
// it's name as a string.  This is used for looking up error codes for
// printing to the console.
//
//*****************************************************************************
tFresultString g_sFresultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_RW_ERROR),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_MKFS_ABORTED)
};

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_sFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
const char *
StringFromFresult(FRESULT fresult) {
    unsigned int uIdx;

    //
    // Enter a loop to search the error code table for a matching
    // error code.
    //
    for(uIdx = 0; uIdx < NUM_FRESULT_CODES; uIdx++)
    {
        //
        // If a match is found, then return the string name of the
        // error code.
        //
        if(g_sFresultStrings[uIdx].fresult == fresult)
        {
            return(g_sFresultStrings[uIdx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a
    // string indicating unknown error.
    //
    return("UNKNOWN ERROR CODE");
}

//*****************************************************************************
//
// This function implements the "ls" command.  It opens the current
// directory and enumerates through the contents, and prints a line for
// each item it finds.  It shows details such as file attributes, time and
// date, and the file size, along with the name.  It shows a summary of
// file sizes at the end along with free space.
//
//*****************************************************************************
int
Cmd_ls(int argc, char *argv[])
{
    unsigned long ulTotalSize;
    unsigned long ulFileCount;
    unsigned long ulDirCount;
    FRESULT fresult;
    FATFS *pFatFs;

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
		INFO("%s",StringFromFresult(fresult));
        return(0);
    }

    ulTotalSize = 0;
    ulFileCount = 0;
    ulDirCount = 0;

    //
    // Give an extra blank line before the listing.
    //
    INFO("");

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            return(fresult);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // If the attribue is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ulDirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ulFileCount++;
            ulTotalSize += g_sFileInfo.fsize;
        }

        //
        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
        //
        INFO("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s",
                    (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                    (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                    (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                    (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                    (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                    (g_sFileInfo.fdate >> 9) + 1980,
                    (g_sFileInfo.fdate >> 5) & 15,
                     g_sFileInfo.fdate & 31,
                    (g_sFileInfo.ftime >> 11),
                    (g_sFileInfo.ftime >> 5) & 63,
                     g_sFileInfo.fsize,
                     g_sFileInfo.fname);
    }   // endfor

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    INFO("\n%4u File(s),%10u bytes total\n%4u Dir(s)",
                ulFileCount, ulTotalSize, ulDirCount);

    //
    // Get the free space.
    //
    fresult = f_getfree("/", &ulTotalSize, &pFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
		INFO("%s",StringFromFresult(fresult));
        return(0);
    }

    //
    // Display the amount of free space that was calculated.
    //
    INFO("%10uK bytes free\n", ulTotalSize * pFatFs->sects_clust / 2);

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of
// a file and prints it to the console.  This should only be used on
// text files.  If it is used on a binary file, then a bunch of garbage
// is likely to printed on the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT fresult;
    unsigned short usBytesRead;

    //
    // First, check to make sure that the current path (CWD), plus
    // the file name, plus a separator and trailing null, will all
    // fit in the temporary buffer that will be used to hold the
    // file name.  The file name must be fully specified, with path,
    // to FatFs.
    //
    if(ustrlen(g_cCwdBuf) + ustrlen(argv[1]) + 1 + 1 > sizeof(g_cTmpBuf))
    {
        INFO("Resulting path name is too long");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    ustrncpy(g_cTmpBuf, g_cCwdBuf,PATH_BUF_SIZE);

    //
    // If not already at the root level, then append a separator.
    //
    if(ustrncmp("/", g_cCwdBuf,PATH_BUF_SIZE))
    {
        ustrncat(g_cTmpBuf, "/",1);
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    ustrncat(g_cTmpBuf, argv[1],ustrlen(argv[1]));

    //
    // Open the file for reading.
    //
    fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return
    // an error.
    //
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it,
    // until the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit
        // in the temporary buffer, including a space for the trailing null.
        //
        fresult = f_read(&g_sFileObject, g_cTmpBuf, sizeof(g_cTmpBuf) - 1,
                         &usBytesRead);

        //
        // If there was an error reading, then print a newline and
        // return the error to the user.
        //
        if(fresult != FR_OK)
        {
            INFO("%s",StringFromFresult(fresult));
            return(fresult);
        }

        //
        // Null terminate the last block that was read to make it a
        // null terminated string that can be used with printf.
        //
        g_cTmpBuf[usBytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
        UARTSend(UART0_BASE,g_cTmpBuf,ustrlen(g_cTmpBuf));

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(usBytesRead == sizeof(g_cTmpBuf) - 1);

    //
    // Return success.
    //
    return(0);
}

int cmd_help(int argc, char *argv[]) {
    cmdline_entry *pEntry;

	INFO("\r\nAvailable commands\r\n------------------");
    // Point at the beginning of the command table.
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->cmd)   {
        INFO("%s%s", pEntry->cmd, pEntry->help);
        pEntry++; // Advance to the next entry in the table.
    }

    return(0);
}


cmdline_entry g_sCmdTable[] = {
    { "help",   cmd_help,      " : Display list of commands" },
    { "ls",     Cmd_ls,      "   : Display list of files" },
 	{ "cat",    Cmd_cat,      "  : Show contents of a text file" },
    { 0, 0, 0 }
};

void blinky(unsigned int count) {

    while( 0 < count-- )    {
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
		vTaskDelay(1000 / portTICK_RATE_MS);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ~GPIO_PIN_0);
		vTaskDelay(1000 / portTICK_RATE_MS);
    }
}


void console( void *pvParameters ) {
	int cmd_status;
    FRESULT fresult;
    //
    // Mount the file system, using logical disk 0.
    //
    fresult = f_mount(0, &g_sFatFs);
    if(fresult != FR_OK) {
        INFO("f_mount error: %s", StringFromFresult(fresult));
		blinky(3);
        return;
    }
	
	UARTSend(UART0_BASE,welcome,ustrlen(welcome));
	for(;;) {
		UARTSend(UART0_BASE, prompt,ustrlen(prompt));
        UARTgets(UART0_BASE,g_cCmdBuf, sizeof(g_cCmdBuf));

  		/* Allocate memory for the structure that holds the state of the
     connection. */
        //
        // Pass the line from the user to the command processor.  It will be
        // parsed and valid commands executed.
        //
        cmd_status = cmdline_process(g_cCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(cmd_status == CMDLINE_BAD_CMD)        {
			UARTSend(UART0_BASE, welcome,ustrlen(welcome));
        } else if(cmd_status == CMDLINE_TOO_MANY_ARGS) {
            UARTSend(UART0_BASE,"Too many arguments for command processor!\r\n",43);
        }
	}
	
}


int main( void ) {
	prvSetupHardware();

	if (pdPASS != xTaskCreate( console, ( signed portCHAR * ) "SDCART", SIM_TASK_STACK_SIZE, NULL, CHECK_TASK_PRIORITY , NULL )) {
		INFO("Cant create console!");
	}

	vTaskStartScheduler(); // Start the scheduler. 

    /* Will only get here if there was insufficient memory to create the idle
    task. */
	for( ;; );
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

	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
	
    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

	uart_init(0, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}
/*-----------------------------------------------------------*/


void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName ) {
	INFO("Stackoverflow task:%s",pcTaskName);
}

void vApplicationTickHook( void ) {
static unsigned portLONG ulTicks = 0;

	/* Called from every tick interrupt.  Have enough ticks passed to make it
	time to perform sd card timer*/
	ulTicks++;
	if( ulTicks >= mainCHECK_DELAY ) {
		ulTicks = 0;
		//
	    // Call the FatFs tick timer.
	    //
	    disk_timerproc();
	}
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
