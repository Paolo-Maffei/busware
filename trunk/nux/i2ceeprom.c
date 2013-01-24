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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

/* driverlib library includes */
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/i2c.h"


void I2CEE_init() {

    // The I2C0 peripheral must be enabled before use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);


    //
    // Configure the pin muxing for I2C1 functions on port B2 and B3.
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    //
    // Select the I2C function for these pins.  This function will also
    // configure the GPIO pins for I2C operation, setting them to
    // open-drain operation with weak pull-ups.
    //
    GPIOPinTypeI2C(GPIO_PORTB_BASE, I2C0SCL_PIN | I2C0SDA_PIN);


    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.  For this example we will use a data rate of 100kbps.
    //
    I2CMasterInitExpClk(I2C0_MASTER_BASE, SysCtlClockGet(), false);

    //
    // Enable the I2C0 slave module.
    //
    I2CSlaveEnable(I2C0_SLAVE_BASE);
}

//*****************************************************************************
// Non Interupt driven Write from the Eeprom.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param data is the value to be written in the Eeprom
// \param reg_address is the memory address which will be written at
//*****************************************************************************
void modee_write_block(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address) {
	int i;
    ///////////////////////////////////////////////////////////////////
    // Start with the control byte (address) to the Eeprom.
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, slave_address, false);
    
    // Place the address to be written in the data register.
    I2CMasterDataPut(I2C0_MASTER_BASE, (unsigned char)(reg_address));
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);    
    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait

	for(i=0; i < (length-1); i++) {
	    I2CMasterDataPut(I2C0_MASTER_BASE, *(data+i));
	    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait
	}
	
    I2CMasterDataPut(I2C0_MASTER_BASE, *(data+i));
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait

	SysCtlDelay(SysCtlClockGet() / 500);    // here must be a delay to wait the write complete or to poll the ACK
}


void I2CEE_write(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address) {
	unsigned long blklen;
	unsigned short offset;
	offset = 0;
	while (length > 0) {
		blklen = length;
		if(blklen > 16) {
			blklen=16;
		}
		modee_write_block(slave_address,data+offset,blklen,reg_address+offset);
		length-=blklen;
		offset+=blklen;
	}
}
//*****************************************************************************
// Non Interupt driven Read from the Eeprom device.
// \param slave_address is the Control byte shift right once. (0x50=0xA0>>1)  
// \param reg_address is the memory address to be read
//*****************************************************************************
void I2CEE_read(unsigned char slave_address, unsigned char *data, unsigned long length, unsigned short reg_address){
	int i;
     unsigned long ret = 0;
    ///////////////////////////////////////////////////////////////////
    // Start with a write to set the address in the Eeprom.
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, slave_address, false);
    
    // Place the address to be written in the data register.
    I2CMasterDataPut(I2C0_MASTER_BASE, (unsigned char)reg_address);
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);    
    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait

    // Put the I2C master into receive mode.
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, slave_address, true);

	i=0;
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait
    ret = I2CMasterDataGet(I2C0_MASTER_BASE);
	*data=ret;
	
	for(i = 1; i < (length-1); i++)	{
		I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
        while(I2CMasterBusy(I2C0_MASTER_BASE)){}
        // get further bytes
        ret = I2CMasterDataGet(I2C0_MASTER_BASE);
        *(data + i) = ret;
	}

    // finish reading sequence
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(I2CMasterBusy(I2C0_MASTER_BASE)){}
    // get last byte
    ret = I2CMasterDataGet(I2C0_MASTER_BASE);
    *(data + i) = ret;
}

unsigned short I2CEE_exists(unsigned char slave_address) {
	unsigned long ret = 0;
	
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, slave_address, false);
    // Place the address to be written in the data register.
    I2CMasterDataPut(I2C0_MASTER_BASE, (unsigned char)0);
    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);    
    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait

    // Put the I2C master into receive mode.
    I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, slave_address, true);

    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
    while(I2CMasterBusy(I2C0_MASTER_BASE)) {}    //wait
	ret=I2CMasterDataGet(I2C0_MASTER_BASE);
	return (ret == 0xFF) ? 0 : 1;
}
