/*************************************************************************
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
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

//*****************************************************************************
//
//! The error code returned when an error occurs that does not need to be
//! handled other than notifying the user.  -1 and -2 are already taken by the
//! command line processor.
//
//*****************************************************************************


#define STATIC_IPADDR_LOW_ID		(0)
#define STATIC_IPADDR_HIGH_ID		(1)
#define STATIC_IPMASK_LOW_ID		(2)
#define STATIC_IPMASK_HIGH_ID		(3)
#define STATIC_IPGW_LOW_ID			(4)
#define STATIC_IPGW_HIGH_ID			(5)
#define IPMODE_ID					(6)
#define UART0_SPEED_HIGH_ID			(7)
#define UART0_SPEED_LOW_ID			(8)
#define UART0_CONFIG_ID				(9)

void console_init(void);

#define TELNETD_CONF_LINELEN 80
#define TELNETD_CONF_NUMLINES 16

struct console_state {
  char *lines[TELNETD_CONF_NUMLINES];
  int line;
};

#define line_malloc (char *)pvPortMalloc(TELNETD_CONF_LINELEN)

#endif /* __CONSOLE__ */