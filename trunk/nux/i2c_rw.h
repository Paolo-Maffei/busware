#ifndef __I2C_RW_H__
#define __I2C_RW_H__

void I2C0_init();

struct module_header {
	unsigned short magic;
	unsigned short vendor;
	unsigned short product;
	unsigned short version;
	unsigned short profile;
	unsigned short crc;
};

struct profile_uart {
	unsigned short magic;
	unsigned long baud;
	unsigned short config;
};

//*****************************************************************************
// Non Interupt driven Write to the Eeprom.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param data is the value to be written in the Eeprom
// \param length is the length of the value to be written in the Eeprom
// \param reg_address is the memory address which will be written at
//*****************************************************************************
void I2C_write(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address);

//*****************************************************************************
// Non Interupt driven Read from the Eeprom device.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param data is the value to be written from the Eeprom
// \param length is the length of the value to be read from Eeprom
// \param reg_address is the memory address to be read
//*****************************************************************************
void I2C_read(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address);

//*****************************************************************************
// Checks existance of the Eeprom device.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \return true if eeprom device exists
//*****************************************************************************
unsigned short I2C_exists(unsigned char slave_address);

#endif /* __I2C_RW_H__ */