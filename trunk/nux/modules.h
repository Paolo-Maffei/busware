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
#ifndef __MODULES_H__
#define __MODULES_H__

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define MODULE1 0x00
#define MODULE2 0x01
#define MODULE4 0x03

#define PROFILE_UART  0x22
#define PROFILE_RELAY 0x14
#define PROFILE_CRC   0x33

#define MAGIC 0x3A

// Before you amend module_info or uart_profile rewite MODEE_write!
struct module_info {
	unsigned short magic;
	unsigned short vendor;
	unsigned short product;
	unsigned short version;
	unsigned short profile;
	unsigned short modres;
	unsigned short dummy2;
	unsigned short crc;
};

struct uart_profile {
	unsigned short profile;
	unsigned long  baud;
	unsigned short config;
	unsigned short buf_size;
};


struct crc_info {
	unsigned short profile;
	unsigned short crc;
	unsigned short crc2;
};

struct uart_info {
	unsigned short profile;
	unsigned long baud;
	unsigned short config;
	unsigned long port;
	unsigned long base;
	unsigned long recv;
	unsigned long sent;
	unsigned long lost;
	unsigned long err;
	unsigned short buf_size;
	xQueueHandle queue;
};

struct relay_info {
	unsigned short profile;
	unsigned long start_value;
	unsigned short negation;
	unsigned short convert;
};


void modules_init();

void module_init(unsigned short module_idx);

unsigned short module_exists(unsigned short module_idx);
unsigned short module_profile_id(unsigned short module_idx);
struct uart_info *get_uart_profile(unsigned short module_idx);
struct crc_info *get_crc_profile(unsigned short module_idx);
struct relay_info *get_relay_profile(unsigned short module_idx);

#endif /* __MODULES_H__ */