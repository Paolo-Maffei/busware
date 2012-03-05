/*****************************************************************************
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
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"

#include "modules.h"
#include "nuxeeprom.h"
#include "crc.h"
#include "relay.h"

extern void uart_init(unsigned short uart_idx, unsigned long baud, unsigned short config);

unsigned char *module[4];

static const unsigned long periph_modres[4] = {
	SYSCTL_PERIPH_GPIOA, SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOC, SYSCTL_PERIPH_GPIOC 
};

static const unsigned long port_reset[4] = {
	GPIO_PORTA_BASE, GPIO_PORTB_BASE, GPIO_PORTC_BASE,GPIO_PORTC_BASE
};

static const unsigned long port_reset_pin[4] = {
	GPIO_PIN_7, GPIO_PIN_1, GPIO_PIN_5,GPIO_PIN_7
};

void modules_init() {
	I2C0_init();
	for(size_t i = MODULE1; i <= MODULE4; i++)	{
		module[i]=NULL;
		module_init(i);
	}

}

void module_init(unsigned short module_idx) {
	unsigned short crc_sum;
	struct module_info *header;
	struct relay_info *relay;

	struct uart_info *uart_config;
	struct uart_profile *uart;
	struct crc_info *crc_error;
	static const unsigned long uart_base[3] = {
		UART0_BASE, UART1_BASE, UART2_BASE
	};
	
	header = (struct module_info *)pvPortMalloc(sizeof(struct module_info));
	uart = (struct uart_profile *)pvPortMalloc(sizeof(struct uart_profile));

	if(I2C_exists(i2c_addresses[module_idx])) {
		I2C_read(i2c_addresses[module_idx], (unsigned char *) header, sizeof(struct module_info), 0);
		crc_sum=crcSlow((unsigned char *)header, sizeof(struct module_info)-sizeof(header->crc));
		if(crc_sum == header->crc) {

			SysCtlPeripheralEnable(periph_modres[module_idx]);
    		// set MOD_RES == Low
    		GPIOPinTypeGPIOOutput(port_reset[module_idx], port_reset_pin[module_idx] );
    		GPIOPinWrite(port_reset[module_idx], port_reset_pin[module_idx] , header->modres);

			if(header->profile == PROFILE_UART) {
				uart_config = (struct uart_info *)pvPortMalloc(sizeof(struct uart_info));
				I2C_read(i2c_addresses[module_idx], (unsigned char *) uart, sizeof(struct uart_profile), sizeof(struct module_info));
				if(uart->profile == PROFILE_UART) {
					uart_config->baud   = uart->baud;
					uart_config->config = uart->config;
					uart_config->buf_size = (uart->buf_size & 0x0f);
#ifdef INCOMPATIBLE_PROFILE
					uart_config->baud = 115200;
					uart_config->config = (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
					uart_config->buf_size = 1;
#endif
				} else {
					uart_config->baud = 115200;
					uart_config->config = (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
					uart_config->buf_size = 1;
				}
				uart_config->port = 1234+module_idx;
				uart_config->profile = PROFILE_UART;
				uart_config->recv    = 0;
				uart_config->sent    = 0;
				uart_config->err     = 0;
				uart_config->lost    = 0;
				uart_config->queue   = xQueueCreate( uart_config->buf_size * UART_QUEUE_SIZE, sizeof(char));
				uart_config->base    = uart_base[module_idx+1];
				uart_init(module_idx+1, uart_config->baud, uart_config->config); // MODULE1 == UART1, MODULE2 == UART2 etc.
				module[module_idx] = (unsigned char *)uart_config;
			} else if (header->profile == PROFILE_RELAY) {
				relay = (struct relay_info *)pvPortMalloc(sizeof(struct relay_info));
				I2C_read(i2c_addresses[module_idx], (unsigned char *) relay, sizeof(struct relay_info), sizeof(struct module_info));
				if(relay->profile != PROFILE_RELAY) {
					relay->profile = PROFILE_RELAY;
					relay->start_value    = 0;
					relay->negation       = 0;
					relay->convert        = 0;
				}
				relay_init(module_idx);
				module[module_idx] = (unsigned char *)relay;
			}
		} else {
			crc_error = (struct crc_info *)pvPortMalloc(sizeof(struct crc_info));
			crc_error->profile = PROFILE_CRC;
			crc_error->crc = header->crc;
			crc_error->crc2 = crc_sum;
			module[module_idx] = (unsigned char *)crc_error;
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

struct relay_info *get_relay_profile(unsigned short module_idx){
	return (struct relay_info *)module[module_idx];
}

unsigned short module_exists(unsigned short module_idx) {
	if(module_idx < MODULE1 || module_idx > MODULE4) {
		return 0;
	}
	return (module[module_idx] == NULL) ? 0 : 1;
}
