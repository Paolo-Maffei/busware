void I2C0_init();

//*****************************************************************************
// Non Interupt driven Write to the Eeprom.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param data is the value to be written in the Eeprom
// \param reg_address is the memory address which will be written at
//*****************************************************************************
void I2C_write(unsigned char slave_address, unsigned char data, unsigned short reg_address);

//*****************************************************************************
// Non Interupt driven Read from the Eeprom device.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param reg_address is the memory address to be read
//*****************************************************************************
unsigned char I2C_read(unsigned char slave_address, unsigned short reg_address);