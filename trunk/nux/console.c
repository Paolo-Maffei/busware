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
#include "stdlib.h"
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "utils/cmdline.h"
#include "utils/ustdlib.h"
#include "softeeprom.h"
#include "console.h"




extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);
extern int UARTgets(unsigned long ulBase, char *pcBuf, unsigned long ulLen);

/*****************************************************************************
! The buffer that holds the command line entry.
*****************************************************************************/

struct console_state *cmd_out;

const portCHAR * const welcome = "\r\nNUX console V1.0\r\nType \'help\' for help.\r\n";
const portCHAR * const prompt = "\r\nnux> ";

/* Helper function to print a line to a virtual screen
*/
int cmd_print(const char *pcString, ...) {
	va_list vaArgP;
	char *str;
	int len;
	
	str= (char *)pvPortMalloc(TELNETD_CONF_LINELEN);
	if(str == NULL) {
		return ERROR_MEM;
	}

    va_start(vaArgP, pcString); // Start the varargs processing.
    len = uvsnprintf(str, TELNETD_CONF_LINELEN, pcString, vaArgP);
    va_end(vaArgP);

    cmd_out->line++;
	cmd_out->lines[cmd_out->line] = str;
	
	return len;
}

/*****************************************************************************
! Outputs the error.
!
! This function will output an error message for the given error code and
! enter an infinite loop.  The message is sent out via UART0.
!
! \param ulError is the error code returned by the soft EEPROM drivers.
!
! \return ERROR_MEM.
*****************************************************************************/
int output_error(unsigned long ulError) {
    //
    // Switch and output based on the error code.
    //
    switch(ulError & 0x7FFF) {
        case ERR_NOT_INIT:
            cmd_print("\r\nERROR: Soft EEPROM not initialized!");
            break;
        case ERR_ILLEGAL_ID:
            cmd_print("\r\nERROR: Illegal ID used!");
            break;
        case ERR_PG_ERASE:
            cmd_print("\r\nERROR: Soft EEPROM page erase error!");
            break;
        case ERR_PG_WRITE:
            cmd_print("\r\nERROR: Soft EEPROM page write error!");
            break;
        case ERR_ACTIVE_PG_CNT:
            cmd_print("\r\nERROR: Active soft EEPROM page count error!");
            break;
        case ERR_RANGE:
            cmd_print("\r\nERROR: Soft EEPROM specified out of range!");
            break;
        case ERR_AVAIL_ENTRY:
            cmd_print("\r\nERROR: Next available entry error!");
            break;
        case ERR_TWO_ACTIVE_NO_FULL:
            cmd_print("\r\nERROR: Two active pages found but not full!");
            break;
        default:
            cmd_print("\r\nERROR: Unidentified Error");
            break;
    }
	                       
    //
    // Did the error occur during the swap operation?
    //
    if(ulError & ERR_SWAP) {
        //
        // Indicate that the error occurred during the swap operation.
        //
        cmd_print("\r\nOccurred during the swap operation.");
    }
    
	return ERROR_MEM;
}


int cmd_quit(int argc, char *argv[]) {
    cmd_print("\r\nBye.");
	
	return CMDLINE_QUIT;
}
/*****************************************************************************

! Implements the clear command.
!
! This function implements the "c" (clear) command.  It clears the contents
! of the soft EEPROM. An argument count (argc) of zero is expected.
!
! \param argc is the argument count from the command line.
!
! \param argv is a pointer to the argument buffer from the command line.
!
! \return A value of 0 indicates that the command was successful.  A non-zero
! value indicates a failure.

*****************************************************************************/
int cmd_clear(int argc, char *argv[]) {
    long lEEPROMRetStatus;
    lEEPROMRetStatus = SoftEEPROMClear();
    
    if(lEEPROMRetStatus != 0) {
        cmd_print("\rAn error occurred during a soft EEPROM clear operation");
        return output_error(lEEPROMRetStatus);
    }
    return(0);
}


int cmd_ipmode(int argc, char *argv[]) {
	tBoolean found;
    long lEEPROMRetStatus;
    unsigned short usdata;

	if (argc < 2) {
	    lEEPROMRetStatus = SoftEEPROMRead(IPMODE_ID, &usdata, &found);
		
	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\rAn error occurred during a soft EEPROM read operation");
	        return output_error(lEEPROMRetStatus);
	    }
		usdata = found ? usdata : 1;
		cmd_print("ip mode : %s", usdata == 1 ? "dhcp" : "static");
		
		return(0);
		
	} else {
		if(ustrncmp(argv[1],"dhcp",4) == 0)	{
			usdata=1;
		} else {
			usdata=0;
		}
	    lEEPROMRetStatus = SoftEEPROMWrite(IPMODE_ID, usdata);

	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\r\nAn error occurred during a soft EEPROM write operation");
	        return output_error(lEEPROMRetStatus);
	    }
	}
	return(0);
}

/*
 Implements the ip address command.

 This function implements the "ipaddr" (ip address) set/display command.  

*/
int cmd_static_ipaddr(int argc, char *argv[]) {
	tBoolean found;
    long lEEPROMRetStatus;
    unsigned short usdata,usdata2;

	if (argc < 2) {
	    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPADDR_LOW_ID, &usdata, &found);
		
	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\r\nAn error occurred during a soft EEPROM read operation");
	        return output_error(lEEPROMRetStatus);
	    }
		if(found) {
		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPADDR_HIGH_ID, &usdata2, &found);
	        cmd_print("ip addr: %d.%d.%d.%d", ((usdata2 >> 8) & 0xff),((usdata2 >> 0) & 0xff),((usdata >> 8) & 0xff),((usdata >> 0) & 0xff));

		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPMASK_LOW_ID, &usdata, &found);
		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPMASK_HIGH_ID, &usdata2, &found);
	        cmd_print("\r\nmask   : %d.%d.%d.%d", ((usdata2 >> 8) & 0xff),((usdata2 >> 0) & 0xff),((usdata >> 8) & 0xff),((usdata >> 0) & 0xff));

		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPGW_LOW_ID, &usdata, &found);
		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPGW_HIGH_ID, &usdata2, &found);
	        cmd_print("\r\ngateway: %d.%d.%d.%d", ((usdata2 >> 8) & 0xff),((usdata2 >> 0) & 0xff),((usdata >> 8) & 0xff),((usdata >> 0) & 0xff));
			return(0);
		}
		cmd_print("\r\nNo static ip address stored.");
	} else if (argc == 4){
	    usdata2 = atoi(strtok(argv[1],".")) << 8;
	    usdata2 |= atoi(strtok(NULL,"."));
	    lEEPROMRetStatus = SoftEEPROMWrite(STATIC_IPADDR_HIGH_ID, usdata2);

	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\rAn error occurred during a soft EEPROM write operation");
	        return output_error(lEEPROMRetStatus);
	    }
	    usdata2 = atoi(strtok(NULL,".")) << 8;
	    usdata2 |= atoi(strtok(NULL,""));
	    SoftEEPROMWrite(STATIC_IPADDR_LOW_ID, usdata2);

	    usdata2 = atoi(strtok(argv[2],".")) << 8;
	    usdata2 |= atoi(strtok(NULL,"."));
	    SoftEEPROMWrite(STATIC_IPMASK_HIGH_ID, usdata2);

	    usdata2 = atoi(strtok(NULL,".")) << 8;
	    usdata2 |= atoi(strtok(NULL,""));
	    SoftEEPROMWrite(STATIC_IPMASK_LOW_ID, usdata2);

	    usdata2 = atoi(strtok(argv[3],".")) << 8;
	    usdata2 |= atoi(strtok(NULL,"."));
	    SoftEEPROMWrite(STATIC_IPGW_HIGH_ID, usdata2);

	    usdata2 = atoi(strtok(NULL,".")) << 8;
	    usdata2 |= atoi(strtok(NULL,""));
	    SoftEEPROMWrite(STATIC_IPGW_LOW_ID, usdata2);

	} else {
        cmd_print("Wrong number of arguments\r\n");
        return(ERROR_UNHANDLED);
	}
	return(0);
}


/*
 Implements the ip address command.

 This function implements the "ipaddr" (ip address) set/display command.  

*/
int cmd_uartmode(int argc, char *argv[]) {
	tBoolean found;
    long lEEPROMRetStatus;
    unsigned short uart_base,data,data2,uart_len,uart_stop;
	unsigned long uart_speed;
	char uart_parity;
	
	if (!(argc == 2 || argc == 6)) {
        cmd_print("Wrong number of arguments\r\n");
        return(ERROR_UNHANDLED);
	}

	if (argc == 2) {
		uart_base = ustrtoul(argv[1],NULL,0);
		if(uart_base > 4) {
			cmd_print("Wrong uart device: UART%d. Valid devices are UART0...UART4\r\n", uart_base);
			return (ERROR_UNHANDLED);
		}
		SoftEEPROMRead(UART0_SPEED_HIGH_ID + uart_base, &data, &found);
	    if (found) {
			SoftEEPROMRead(UART0_SPEED_LOW_ID + uart_base, &data2, &found);
			uart_speed =  (data << 16 & 0xFFFF0000) | (data2 & 0x0000FFFF);
			
			SoftEEPROMRead(UART0_CONFIG_ID + uart_base, &data, &found);
			switch(data & UART_CONFIG_WLEN_MASK) {
				case UART_CONFIG_WLEN_8: { uart_len=8; break;}
				case UART_CONFIG_WLEN_7: { uart_len=7; break;}
				case UART_CONFIG_WLEN_6: { uart_len=6; break;}
				case UART_CONFIG_WLEN_5: { uart_len=5; break;}
				default: {uart_len=0;}
            }
			switch(data & UART_CONFIG_STOP_MASK) {
				case UART_CONFIG_STOP_ONE: { uart_stop=1; break;}
				case UART_CONFIG_STOP_TWO: { uart_stop=2; break;}
				default: {uart_stop=0;}
			}
			
			switch(data & UART_CONFIG_PAR_MASK) {
				case UART_CONFIG_PAR_NONE: { uart_parity='N'; break;}
				case UART_CONFIG_PAR_EVEN: { uart_parity='E'; break;}
				case UART_CONFIG_PAR_ODD: { uart_parity='O'; break;}
				case UART_CONFIG_PAR_ONE: { uart_parity='1'; break;}
				case UART_CONFIG_PAR_ZERO: { uart_parity='0'; break;}
				default: {uart_parity='X';}
			}
			
			cmd_print("UART%d %d %d %c %d", uart_base, uart_speed,  uart_len , uart_parity, uart_stop );
		} else {
			cmd_print("UART%d %d %d %c %d", uart_base, 115200, 8, 'N' , 1);
		}
	} else {
		uart_base = ustrtoul(argv[1],NULL,0);
		if(uart_base > 4) {
			cmd_print("Wrong uart device: UART%d. Valid devices are UART0...UART4\r\n", uart_base);
			return (ERROR_UNHANDLED);
		}
		
		uart_speed= ustrtoul(argv[2],NULL,0);
		switch (*argv[3]) {
			case '5': {data=UART_CONFIG_WLEN_5; break;}
			case '6': {data=UART_CONFIG_WLEN_6; break;}
			case '7': {data=UART_CONFIG_WLEN_7; break;}
			case '8': {data=UART_CONFIG_WLEN_8; break;}
			default: {cmd_print("Invalid parameter len: %s", argv[3]); return (ERROR_UNHANDLED); }
		}

		switch (*argv[4]) {
			case 'N': {data|=UART_CONFIG_PAR_NONE; break;}
			case 'E': {data|=UART_CONFIG_PAR_EVEN; break;}
			case 'O': {data|=UART_CONFIG_PAR_ODD; break;}
			case '1': {data|=UART_CONFIG_PAR_ONE; break;}
			case '0': {data|=UART_CONFIG_PAR_ZERO; break;}

			default: {cmd_print("Invalid parameter parity: %s", argv[4]); return (ERROR_UNHANDLED); }
		}
		switch (*argv[5]) {
			case '1': {data|=UART_CONFIG_STOP_ONE; break;}
			case '2': {data|=UART_CONFIG_STOP_TWO; break;}

			default: {cmd_print("Invalid parameter stop: %s", argv[5]); return (ERROR_UNHANDLED); }
		}

		lEEPROMRetStatus=SoftEEPROMWrite(UART0_SPEED_HIGH_ID + uart_base, uart_speed >> 16);
	    if(lEEPROMRetStatus != 0) {
			cmd_print("\r\nAn error occurred during a soft EEPROM write operation");
			return output_error(lEEPROMRetStatus);
		}
		SoftEEPROMWrite(UART0_SPEED_LOW_ID + uart_base, uart_speed );

		SoftEEPROMWrite(UART0_CONFIG_ID + uart_base, data);
	}
	return(0);
}

//*****************************************************************************
//
//! Implements the help command.
//!
//! This function implements the "help" command.  It prints a simple list of
//! the available commands with a brief description. An argument count (argc)
//! of zero is expected.
//!
//! \param argc is the argument count from the command line.
//!
//! \param argv is a pointer to the argument buffer from the command line.
//!
//! \return This function always returns a value of 0.
//
//*****************************************************************************
int cmd_help(int argc, char *argv[]) {
    tCmdLineEntry *pEntry;

	cmd_print("\r\nAvailable commands\r\n------------------\r\n");
    // Point at the beginning of the command table.
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)   {
        cmd_print("%s%s\r\n", pEntry->pcCmd, pEntry->pcHelp);
        pEntry++; // Advance to the next entry in the table.
    }

    return(0);
}

/* This function stops clearing the watchdog interrupt. As a consequence
the board will perform a reset.
*/
int cmd_restart(int argc, char *argv[]) {
	extern volatile unsigned short should_reset;
	
	should_reset=1;
	return(0);
}

void print_uart(struct console_state *hs) {
	int i;
	for(i=0;i<=hs->line;i++) {
		UARTSend(UART0_BASE,hs->lines[i], ustrlen(hs->lines[i]));
		vPortFree(hs->lines[i]);
	}
	hs->line=-1;
}

//*****************************************************************************
//
//! The table that holds the command names, implementing functions, and brief
//! description.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] = {
    { "help",   cmd_help,      " : Display list of commands" },
    { "h",      cmd_help,   "    : alias for help" },
    { "?",      cmd_help,   "    : alias for help" },
    { "clear",  cmd_clear,  "    : Reset soft EEPROM - Usage: clear" },
    { "restart",  cmd_restart,  "    : Restart software  - Usage: restart" },
    { "ipaddr", cmd_static_ipaddr,  ": set/display static ipaddr address - Usage: ipaddr [addr mask gateway]" },
    { "ipmode", cmd_ipmode, ": set/display ip acquisition mode - Usage: ipmode [dhcp|static]" },
    { "uart",   cmd_uartmode, ": set/display uart - Usage: uart <id> <speed> <len> <stop> <parity>" },

    { "quit",   cmd_quit,   "    : Quit console" },

    { 0, 0, 0 }
};



void console( void *pvParameters ) {
	int cmd_status;
	char *cmd_buf;

	cmd_buf = (char *)pvPortMalloc(TELNETD_CONF_LINELEN);
	
	UARTSend(UART0_BASE,welcome,ustrlen(welcome));
	for(;;) {
		UARTSend(UART0_BASE, prompt,ustrlen(prompt));

        UARTgets(UART0_BASE,cmd_buf, TELNETD_CONF_LINELEN);

  		/* Allocate memory for the structure that holds the state of the
     connection. */
  		cmd_out = (struct console_state *)pvPortMalloc(sizeof(struct console_state));
		cmd_out->line=-1;
        //
        // Pass the line from the user to the command processor.  It will be
        // parsed and valid commands executed.
        //
        cmd_status = CmdLineProcess(cmd_buf);

		print_uart(cmd_out);
		vPortFree(cmd_out);
        
        //
        // Handle the case of bad command.
        //
        if(cmd_status == CMDLINE_BAD_CMD)        {
			UARTSend(UART0_BASE, welcome,ustrlen(welcome));
        } else if(cmd_status == CMDLINE_TOO_MANY_ARGS) {
            UARTSend(UART0_BASE,"Too many arguments for command processor!\r\n",43);
        }
	}
	
	vPortFree(cmd_buf);
}

