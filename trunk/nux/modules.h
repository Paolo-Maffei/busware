#ifndef __MODULES_H__
#define __MODULES_H__

#define MODULE1 0x00
#define MODULE4 0x03

void modules_init();

void module1_init();

unsigned short module_exists(unsigned short module_idx);
unsigned short module_profile_id(unsigned short module_idx);
struct uart_info *get_uart_profile(unsigned short module_idx);
struct crc_info *get_crc_profile(unsigned short module_idx);

#endif /* __MODULES_H__ */