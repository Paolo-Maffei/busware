/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Hardware library includes. */
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#include "modules.h"
#include "i2c_rw.h"
#include "crc.h"


unsigned int module_uart_avail;
unsigned int module_uart_port[4];

void modules_init() {
	I2C0_init();
	module_uart_avail=0;
	
	module1_init();
}

void module1_init() {
	unsigned short crc_sum;
	struct module_info *header;
	struct uart_info *profile;
	extern unsigned int stats_crc_error;
	
	
	header = (struct module_info *)pvPortMalloc(sizeof(struct module_info));
	if(I2C_exists(SLAVE_ADDRESS_MODULE1)) {
		I2C_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) header, sizeof(struct module_info), 0);
		crc_sum=crcSlow((unsigned char *)header, sizeof(struct module_info)-sizeof(header->crc));
		if(crc_sum == header->crc) {

    		// set MOD_RES == Low
    		GPIOPinTypeGPIOOutput( GPIO_PORTA_BASE, GPIO_PIN_7 );
    		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, header->modres);

			if(header->profile == PROFILE_UART) {
				module_uart_avail |= MODULE1; // Modul exists

				profile = (struct uart_info *)pvPortMalloc(sizeof(struct uart_info));
				I2C_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) profile, sizeof(struct uart_info), sizeof(struct module_info));
				if(profile->magic == MAGIC) {
					UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), profile->baud, profile->config);
					module_uart_port[0] = profile->port;
				} else {
					UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
					module_uart_port[0] = 1234;
				}
				vPortFree(profile);
			}
		} else {
			stats_crc_error = 1;
		}
	}
	
	
	vPortFree(header);
}