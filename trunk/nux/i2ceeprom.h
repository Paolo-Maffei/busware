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

#ifndef __I2CEEPROM_H__
#define __I2CEEPROM_H__


#define SLAVE_ADDRESS_MODULE1 0x50
#define SLAVE_ADDRESS_MODULE2 0x51
#define SLAVE_ADDRESS_MODULE3 0x52
#define SLAVE_ADDRESS_MODULE4 0x53

static const unsigned char i2c_addresses[4] = {
	SLAVE_ADDRESS_MODULE1, SLAVE_ADDRESS_MODULE2, SLAVE_ADDRESS_MODULE3, SLAVE_ADDRESS_MODULE4
};

void I2CEE_init();



//*****************************************************************************
// Non Interupt driven Write to the Eeprom.
// The eeprom cant write more then 16 bytes at once!!!!
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param data is the value to be written in the Eeprom
// \param length is the length of the value to be written in the Eeprom
// \param reg_address is the memory address which will be written at
//*****************************************************************************
void I2CEE_write(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address);

//*****************************************************************************
// Non Interupt driven Read from the Eeprom device.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param data is the value to be written from the Eeprom
// \param length is the length of the value to be read from Eeprom
// \param reg_address is the memory address to be read
//*****************************************************************************
void I2CEE_read(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address);

//*****************************************************************************
// Checks existance of the Eeprom device.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \return true if eeprom device exists
//*****************************************************************************
unsigned short I2CEE_exists(unsigned char slave_address);

#endif /* __I2CEEPROM_H__ */