#ifndef _BOARD_H
#define _BOARD_H

#ifdef CSM
#include "board_CSM.h"
#endif

#ifdef ZCSM
#include "board_CSM.h"
#endif

#ifdef RWE_PSS
#include "board_RWE_PSS.h"
#endif

#ifdef HM_LC_Sw1_PI
#include "board_HM_LC-Sw1-PI.h"
#endif

#ifdef PWMBOX
#include "board_PWMBOX.h"
#endif

#ifdef CUNO2
#include "board_CUNO2.h"
#endif

#ifdef HM_OU_LED16
#include "board_HM_OU_LED16.h"
#endif

#endif

