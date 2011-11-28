//*****************************************************************************
//
// eeprom.h - This file contains the definitions and prototypes used by
//                 the eeprom emulation API.
//
// Copyright (c) 2006-2009 Luminary Micro, Inc.  All rights reserved.
// 
// Software License Agreement
// 
// Luminary Micro, Inc. (LMI) is supplying this software for use solely and
// exclusively on LMI's microcontroller products.
// 
// The software is owned by LMI and/or its suppliers, and is protected under
// applicable copyright laws.  All rights are reserved.  Any use in violation
// of the foregoing restrictions may subject the user to criminal sanctions
// under applicable laws, as well as to civil liability for the breach of the
// terms and conditions of this license.
// 
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// LMI SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 495 of the Stellaris boot loader.
//
//*****************************************************************************

#ifndef __EEPROM_H__
#define __EEPROM_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \addtogroup softeeprom_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The error code used to indicate that an EEPROM operation has been requested
//! without the EEPROM being initialized.  This error code can be returned by
//! SoftEEPROMWrite(), SoftEEPROMRead(), and SoftEEPROMClear().
//
//*****************************************************************************
#define ERR_NOT_INIT			0x0001

//*****************************************************************************
//
//! The error code used to indicate that a read/write request has been made
//! from/to an illegal identifier.  This error code can be returned by
//! SoftEEPROMWrite() and SoftEEPROMRead().
//
//*****************************************************************************
#define ERR_ILLEGAL_ID  		0x0002

//*****************************************************************************
//
//! The error code used to indicate that an error occurred while erasing an
//! EEPROM page.  This error code can be returned by SoftEEPROMInit(),
//! SoftEEPROMClear(), and SoftEEPROMWrite().  When returned by
//! SoftEEPROMWrite(), the actual returned value is ERR_PG_ERASE | ERR_SWAP.
//
//*****************************************************************************
#define ERR_PG_ERASE			0x0003

//*****************************************************************************
//
//! The error code used to indicate that an error occurred while writing to an
//! EEPROM page.  This can occur while writing EEPROM page status words and 
//! EEPROM entries.  This error code can be returned by SoftEEPROMInit(),
//! SoftEEPROMClear(), and SoftEEPROMWrite().  SoftEEPROMWrite() can return
//! both ERR_PG_WRITE and ERR_PG_WRITE | ERR_SWAP.  
//
//*****************************************************************************
#define ERR_PG_WRITE		    0x0004

//*****************************************************************************
//
//! The error code returned by SoftEEPROMInit() when more than two active pages
//! are found upon initialization.
//
//*****************************************************************************
#define ERR_ACTIVE_PG_CNT		0x0005

//*****************************************************************************
//
//! The error code returned by SoftEEPROMInit() when the EEPROM region is
//! specified to be outside the Flash range.
//
//*****************************************************************************
#define ERR_RANGE  		        0x0006

//*****************************************************************************
//
//! The error code returned by SoftEEPROMWrite() if a page swap occurs and
//! the next available entry is outside of the new page. If a 1-K page size is
//! is used (1024/4 - 2 = 254 entries) and all 255 IDs are used (0-254), then
//! it is possible to swap to a new page and not have any available entries in
//! the new page.  This can be avoided by configuring the EEPROM appropriately.
//! The actual returned value is ERR_AVAIL_ENTRY | ERR_SWAP.
//
//*****************************************************************************
#define ERR_AVAIL_ENTRY	        0x0007

//*****************************************************************************
//
//! The error code returned by SoftEEPROMInit() when 2 active pages are found
//! but neither of them are full.  The only time there should be 2 active pages
//! is when a reset or power-down occurs at a specific time interval during
//! PageSwap().  However, PageSwap() should only be called when a page is full.
//
//*****************************************************************************
#define ERR_TWO_ACTIVE_NO_FULL  0x0008

//*****************************************************************************
//
//! Part of the error code returned by SoftEEPROMWrite() if an error occurs
//! during the swap portion of the write operation. ERR_SWAP is ORed in with 
//! the error code encountered during the swap operation (ERR_PG_ERASE,
//! ERR_PG_WRITE, or ERR_AVAIL_ENTRY).  This is provided since the user never
//! calls the swap operation directly and otherwise would not know that the
//! error occurred during a swap operation.
//
//*****************************************************************************
#define ERR_SWAP				0x8000	

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************


//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern long SoftEEPROMInit(unsigned long ulStart, unsigned long ulEnd,
                           unsigned long ulSize);
extern long SoftEEPROMRead(unsigned char usID, unsigned short *pusData, 
                           tBoolean *pbFound);
extern long SoftEEPROMWrite(unsigned short usID, unsigned short usData);
extern long SoftEEPROMClear(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __EEPROM_H__
