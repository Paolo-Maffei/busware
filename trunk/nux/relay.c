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


static const unsigned long periph_relay[4][2] = {
	{SYSCTL_PERIPH_GPIOA, SYSCTL_PERIPH_GPIOD},
	{SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOD},
	{SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOD},
	{SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOD}
};

static const unsigned long port_relay[4][2] = {
	{GPIO_PORTA_BASE, GPIO_PORTD_BASE},
	{GPIO_PORTB_BASE, GPIO_PORTD_BASE},
	{GPIO_PORTB_BASE, GPIO_PORTD_BASE},
	{GPIO_PORTB_BASE, GPIO_PORTD_BASE}
};

static const unsigned char port_relay_pin[4][2] = {
	{GPIO_PIN_3, GPIO_PIN_4},
	{GPIO_PIN_4, GPIO_PIN_5},
	{GPIO_PIN_5, GPIO_PIN_6},
	{GPIO_PIN_6, GPIO_PIN_7}
};

void relay_init(unsigned short module_idx) {
	SysCtlPeripheralEnable(periph_relay[module_idx][0]);
	SysCtlPeripheralEnable(periph_relay[module_idx][1]);
	GPIOPinTypeGPIOInput(port_relay[module_idx][0], port_relay_pin[module_idx][0] );
	GPIOPinTypeGPIOInput(port_relay[module_idx][1], port_relay_pin[module_idx][1] );
	GPIOPinTypeGPIOOutput(port_relay[module_idx][0], port_relay_pin[module_idx][0] );
	GPIOPinTypeGPIOOutput(port_relay[module_idx][1], port_relay_pin[module_idx][1] );

}

unsigned long relay_read(unsigned short module_idx, unsigned short pin_idx) {
	return GPIOPinRead(port_relay[module_idx][pin_idx],port_relay_pin[module_idx][pin_idx] );
}

void relay_write(unsigned short module_idx, unsigned short pin_idx, unsigned char value) {
	GPIOPinWrite(port_relay[module_idx][pin_idx],port_relay_pin[module_idx][pin_idx] , value);
}

unsigned short pin_exists(unsigned short module_idx, unsigned short pin_idx) {
	return (pin_idx < 0 || pin_idx > 1) ? 0 : 1;
}