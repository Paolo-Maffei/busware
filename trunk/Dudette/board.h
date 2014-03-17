#ifndef _BOARD_H
#define _BOARD_H

// RWE Smarthome Funk-Zwischenstecker-Schaltaktor (PSS)
// keep button pressed during plugging in the device
#ifdef RWE_PSS
#include "board_RWE_PSS.h"
#endif

// busware - CC1101-Serial-Module
// tie BL_SEL to GND while booting
#ifdef CSM
#include "board_CSM.h"
#endif

// busware - Arduino ext with zCSM
// tie (digital 6) to LOW while booting (LOW->HIGH on digital 7) - with jumpers in place!
#ifdef ARDCSM
#include "board_CSM.h"
#endif

// busware - CC1101-Serial-Module (ZWIR footprint)
// tie BL_SEL to VDD while booting
#ifdef ZCSM
#include "board_CSM.h"
#endif

// busware - 2 channel PWMBOX
// keep PROGRAM-button pressed while powering up
#ifdef PWMBOX
#include "board_PWMBOX.h"
#endif

// busware - CC1101-USB-Network-Onewire device
// keep PROGRAM-button pressed while powering up
#ifdef CUNO2
#include "board_CUNO2.h"
#endif

// busware - CC1101-Onewire-Clock device for RPi
// pull GPIO18 low during startup or reset by GPIO17
#ifdef COC
#include "board_COC.h"
#endif

// busware - Stackable CC1101 device for RPi
// pull GPIO18 low during startup or reset by GPIO17
// in addition: press the microbutton on desired extension
#ifdef SCC
#include "board_SCC.h"
#endif

// busware - CC1101-Clock-Display device for RPi
// pull GPIO22 low during startup or reset by GPIO17
#ifdef CCD
#include "board_CCD.h"
#endif

// busware - ceiling sensor board
// device will always check for Airdude while booting
#ifdef CSB
#include "board_CSB.h"
#endif

// HomeMatic Funk-Zwischenstecker-Schaltaktor 1fach
// Artikel-Nr.: 68-10 57 88
// keep button pressed during plugging in the device
#ifdef HM_LC_Sw1_PI
#include "board_HM_LC-Sw1-PI.h"
#endif

// HomeMatic Funk-Unterputz-Schaltaktor 1fach
// Artikel-Nr.: 68-767-93
// keep button pressed during plugging in the device
#ifdef HM_LC_Sw1_FM
#include "board_HM_LC-Sw1-FM.h"
#endif

// Homematic Funk-Statusanzeige LED16
// Artikel-Nr.: 68-10 47 98 
// keep middle key (send/reset) pressed during powering up device
#ifdef HM_OU_LED16
#include "board_HM_OU_LED16.h"
#endif

// Homematic Funk-Schaltaktor 4fach Hutschienenmontage
// Artikel-Nr.: 68-09 18 36 or 68-917-50 
// keep channel-1-key pressed during powering up device
#ifdef HM_LC_Sw4_DR
#include "board_HM_LC_Sw4_DR.h"
#endif

#endif

