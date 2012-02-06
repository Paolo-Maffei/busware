/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Hardware library includes. */
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"

#include "modules.h"
#include "i2c_rw.h"
#include "crc.h"


unsigned char *module[4];

void modules_init() {
	I2C0_init();
	for(size_t i = MODULE1; i <= MODULE4; i++)	{
		module[i]=NULL;
	}
	module1_init();
}

void module1_init() {
	unsigned short crc_sum;
	struct module_info *header;
	struct uart_info *uart_config;
	struct uart_profile *uart;
	struct crc_info *crc_error;
	
	
	header = (struct module_info *)pvPortMalloc(sizeof(struct module_info));
	uart = (struct uart_profile *)pvPortMalloc(sizeof(struct uart_profile));

	if(I2C_exists(SLAVE_ADDRESS_MODULE1)) {
		I2C_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) header, sizeof(struct module_info), 0);
		crc_sum=crcSlow((unsigned char *)header, sizeof(struct module_info)-sizeof(header->crc));
		if(crc_sum == header->crc) {

    		// set MOD_RES == Low
    		GPIOPinTypeGPIOOutput( GPIO_PORTA_BASE, GPIO_PIN_7 );
    		GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, header->modres);

			if(header->profile == PROFILE_UART) {
				uart_config = (struct uart_info *)pvPortMalloc(sizeof(struct uart_info));
				I2C_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) uart, sizeof(struct uart_profile), sizeof(struct module_info));
				if(uart->profile == PROFILE_UART) {
					uart_config->baud   = uart->baud;
					uart_config->config = uart->config;
					uart_config->port   = uart->port;
				} else {
					uart_config->baud = 115200;
					uart_config->config = (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
					uart_config->port = 1234;
				}
				uart_config->profile = PROFILE_UART;
				uart_config->recv    = 0;
				uart_config->sent    = 0;
				uart_config->err     = 0;
				uart_config->lost    = 0;
				uart_config->queue   = xQueueCreate( UART_QUEUE_SIZE, sizeof(char));
				uart_config->base    = UART1_BASE;
				UARTConfigSetExpClk(uart_config->base, SysCtlClockGet(), uart_config->baud, uart_config->config);
				UARTFIFOLevelSet(uart_config->base, UART_FIFO_TX7_8, UART_FIFO_RX7_8);
				IntEnable(INT_UART1);
			    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT | UART_INT_OE);
			
				module[0] = (unsigned char *)uart_config;
			}
		} else {
			crc_error = (struct crc_info *)pvPortMalloc(sizeof(struct crc_info));
			crc_error->profile = PROFILE_CRC;
			crc_error->crc = header->crc;
			crc_error->crc2 = crc_sum;
		}
	}
	
	vPortFree(uart);
	vPortFree(header);
}

unsigned short module_profile_id(unsigned short module_idx) {
	return *(unsigned short *)module[module_idx];
}

struct uart_info *get_uart_profile(unsigned short module_idx) {
	return (struct uart_info *)module[module_idx];
}

struct crc_info *get_crc_profile(unsigned short module_idx){
	return (struct crc_info *)module[module_idx];
}
unsigned short module_exists(unsigned short module_idx) {
	return (module[module_idx] == NULL) ? 0 : 1;
}
