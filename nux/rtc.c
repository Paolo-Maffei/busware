/*****************************************************************************
Copyright (C) 2012  busware

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

#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/sysctl.h"


void rtc_init() {
    unsigned long count = 0;

    // Enable the Hibernation module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
    
    // Per an erratum, sometimes on wake the Hibernation module control
    // register will be cleared when it should not be.  As a workaround a
    // location in the non-volatile data area can be read instead.  This data
    // area is cleared to 0 on reset, so if the first location is non-zero then
    // the Hibernation module is in use.  In this case, re-enable the
    // Hibernation module which will ensure that the control register bits have
    // the proper value.
    //

    //
    // Enable the Hibernation module.  This should always be called, even if
    // the module was already enabled, because this function also initializes
    // some timing parameters.
    //
    HibernateEnableExpClk(SysCtlClockGet());
    HibernateClockSelect(HIBERNATE_CLOCK_SEL_RAW);

    // Allow time for the crystal to power up.
    //SysCtlDelay(SysCtlClockGet() / 500);;

    //
    // Increment the hibernation count, and store it in the battery backed
    // memory.
    //
    HibernateRTCEnable();

    HibernateRTCSet(0);

    //count=5;
    //HibernateDataSet(&count, 1);
    //HibernateDataGet(&count, 1);
}
