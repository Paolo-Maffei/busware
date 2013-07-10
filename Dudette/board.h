#ifndef _BOARD_H
#define _BOARD_H

#ifdef CSM
#include "board_CSM.h"
#endif

#ifdef ZCSM
#include "board_CSM.h"
#endif

// RWE Smarthome Funk-Zwischenstecker-Schaltaktor (PSS)
// keep button pressed during plugging in the device
#ifdef RWE_PSS
#include "board_RWE_PSS.h"
#endif

// HomeMatic Funk-Zwischenstecker-Schaltaktor 1fach
// Artikel-Nr.: 68-10 57 88
// keep button pressed during plugging in the device
#ifdef HM_LC_Sw1_PI
#include "board_HM_LC-Sw1-PI.h"
#endif

#ifdef PWMBOX
#include "board_PWMBOX.h"
#endif

#ifdef CUNO2
#include "board_CUNO2.h"
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

