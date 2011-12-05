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

#ifndef __CMDLINE_H__
#define __CMDLINE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define CMDLINE_BAD_CMD         (-1)
#define CMDLINE_TOO_MANY_ARGS   (-2)
#define ERROR_UNHANDLED		    (-3)
#define ERROR_MEM			    (-4)
#define CMDLINE_QUIT		    (-5)


//*****************************************************************************
//
// Command line function callback type.
//
//*****************************************************************************
typedef int (*pfnCmdLine)(int argc, char *argv[]);

typedef struct {
    const char *cmd;
    pfnCmdLine pfnCmd;
    const char *help;
} cmdline_entry;

extern cmdline_entry g_sCmdTable[];

extern int cmdline_process(char *pcCmdLine);

#ifdef __cplusplus
}
#endif

#endif // __CMDLINE_H__
