#ifndef __MODULES_H__
#define __MODULES_H__

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define MODULE1 0x00
#define MODULE4 0x03

// Before you amend module_info or uart_profile rewite I2C_write!
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
	unsigned long baud;
	unsigned short config;
	unsigned long port;
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
	xQueueHandle queue;
};


void modules_init();

void module1_init();

unsigned short module_exists(unsigned short module_idx);
unsigned short module_profile_id(unsigned short module_idx);
struct uart_info *get_uart_profile(unsigned short module_idx);
struct crc_info *get_crc_profile(unsigned short module_idx);

#endif /* __MODULES_H__ */