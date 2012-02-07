#ifndef __I2C_RW_H__
#define __I2C_RW_H__


#define SLAVE_ADDRESS_MODULE1 0x50
#define SLAVE_ADDRESS_MODULE2 0x51
#define SLAVE_ADDRESS_MODULE3 0x52
#define SLAVE_ADDRESS_MODULE4 0x53

static const unsigned char i2c_addresses[4] = {
	SLAVE_ADDRESS_MODULE1, SLAVE_ADDRESS_MODULE2, SLAVE_ADDRESS_MODULE3, SLAVE_ADDRESS_MODULE4
};

void I2C0_init();



//*****************************************************************************
// Non Interupt driven Write to the Eeprom.
// The eeprom cant write more then 16 bytes at once!!!!
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