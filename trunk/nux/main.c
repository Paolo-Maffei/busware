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
#include "hw_memmap.h"
#include "hw_types.h"
#include "hw_sysctl.h"
#include "hw_ints.h"
#include "hw_watchdog.h"

/* driverlib library includes */
#include "sysctl.h"
#include "gpio.h"
#include "grlib.h"
#include "uart.h"
#include "watchdog.h"

#include "lwip/tcpip.h"
#include "lwiplib.h"
#include "lwip/netif.h"

#include "utils/ustdlib.h"

#include "LWIPStack.h"
#include "ETHIsr.h"
#include "softeeprom.h"
#include "console.h"

/*-----------------------------------------------------------*/


#define mainBASIC_TELNET_STACK_SIZE            ( configMINIMAL_STACK_SIZE * 2 )


#define SIM_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )

/* Task priorities. */
#define CHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )

/* The maximum number of messages that can be waiting for process at any one
time. */
#define UART_QUEUE_SIZE						( 512 )


#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))

/*-----------------------------------------------------------*/
void console( void *pvParameters );
static void prvSetupHardware( void ); // configure the hardware

/*-----------------------------------------------------------*/

/* The queue used to send messages from UART1 device. */
xQueueHandle xUART1Queue;

volatile unsigned short should_reset; // watchdog variable to perform a reboot

/*-----------------------------------------------------------*/

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


int blinky(unsigned int count) {

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
	prvSetupHardware();


	xUART1Queue = xQueueCreate( UART_QUEUE_SIZE, sizeof( portCHAR ) );

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
    volatile unsigned long ulLoop;
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
    // Do a dummy read to insert a few cycles after enabling the peripheral.
    //
    ulLoop = SYSCTL_RCGC2_R;
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
		uart_speed = data << 8;
	    SoftEEPROMRead(UART1_SPEED_LOW_ID, &data2, &found);
		uart_speed = (data << 16 & 0xFFFF0000) | (data2 & 0x0000FFFF);
	    SoftEEPROMRead(UART1_CONFIG_ID, &data, &found);
	} else {
		uart_speed=38400;
		data = (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	}

    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), uart_speed, data);

    IntEnable(INT_UART1);

    //UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);


    
    //
    // Check to see if an error occurred.
    //
    if(SoftEEPROMInit(0x1F000, 0x20000, 0x800) != 0)  {
		LWIPDebug("SoftEEPROM initialisation failed.");
    }


	SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    IntEnable(INT_WATCHDOG); // Enable the watchdog interrupt.
    WatchdogReloadSet(WATCHDOG0_BASE, SysCtlClockGet());
    WatchdogResetEnable(WATCHDOG0_BASE);
    WatchdogEnable(WATCHDOG0_BASE);
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void ) {
}


void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName ) {
	LWIPDebug("Stackoverflow %s",pcTaskName);
}


void LWIPDebug(const char *pcString, ...) {
	va_list vaArgP;
	char *buf;
	int len;

 	buf = (char *)pvPortMalloc(TELNETD_CONF_LINELEN);
    va_start(vaArgP, pcString);
    len = uvsnprintf(buf, TELNETD_CONF_LINELEN, pcString, vaArgP);
	UARTSend(UART0_BASE,buf,len);
    va_end(vaArgP);
	vPortFree(buf);
}

/*
	Watchdog handler. 
*/
void WatchdogIntHandler(void) {
    if (! should_reset) {
	    WatchdogIntClear(WATCHDOG0_BASE);
	}

}
