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


#include "utils/cmdline.h"
#include "utils/vstdlib.h"

//*****************************************************************************
//
// Defines the maximum number of arguments that can be parsed.
//
//*****************************************************************************
#ifndef CMDLINE_MAX_ARGS
#define CMDLINE_MAX_ARGS        8
#endif


int cmdline_process(char *pcCmdLine) {
    static char *argv[CMDLINE_MAX_ARGS + 1];
    char *pcChar;
    int argc;
    int bFindArg = 1;
    cmdline_entry *pCmdEntry;

    //
    // Initialize the argument counter, and point to the beginning of the
    // command line string.
    //
    argc = 0;
    pcChar = pcCmdLine;

    while(*pcChar)  {
        if(*pcChar == ' ')  {
            *pcChar = 0;
            bFindArg = 1;
        }  else  {
            if(bFindArg) {
                //
                // As long as the maximum number of arguments has not been
                // reached, then save the pointer to the start of this new arg
                // in the argv array, and increment the count of args, argc.
                //
                if(argc < CMDLINE_MAX_ARGS) {
                    argv[argc] = pcChar;
                    argc++;
                    bFindArg = 0;
                } else  {
                    return(CMDLINE_TOO_MANY_ARGS);
                }
            }
        }

        pcChar++;
    }

    if(argc)  {
        pCmdEntry = &g_sCmdTable[0];

        while(pCmdEntry->cmd)  {
            if(!ustrncmp(argv[0], pCmdEntry->cmd,ustrlen(pCmdEntry->cmd))) {
                return(pCmdEntry->pfnCmd(argc, argv));
            }

            pCmdEntry++;
        }
    }

    return(CMDLINE_BAD_CMD);
}
