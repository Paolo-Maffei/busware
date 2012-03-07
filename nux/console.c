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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "lwip/netif.h"

#include "driverlib/uart.h"
#include "driverlib/flash.h"
#include "utils/cmdline.h"
#include "utils/vstdlib.h"
#include "softeeprom.h"
#include "console.h"

#include "LWIPStack.h"
#include "modeeprom.h"
#include "modules.h"
#include "crc.h"
#include "relay.h"

extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);
extern int UARTgets(unsigned long ulBase, char *pcBuf, unsigned long ulLen);

/*****************************************************************************
! The buffer that holds the command line entry.
*****************************************************************************/

struct console_state *cmd_out;

const portCHAR * const welcome = "\r\nNUX console V1.0\r\nType \'help\' for help.\r\n";
const portCHAR * const prompt = "\r\nnux> ";

int read_uartmode(int argc, char *argv[]);

/* Helper function to print a line to a virtual screen
*/
int cmd_print(const char *pcString, ...) {
	va_list vaArgP;
	char *str;
	int len=0;
	

	if(cmd_out->line + 1 < TELNETD_CONF_NUMLINES) {
	    cmd_out->line++;

		str= (char *)pvPortMalloc(TELNETD_CONF_LINELEN);
		if(str == NULL) {
			return ERROR_MEM;
		}

	    va_start(vaArgP, pcString); // Start the varargs processing.
	    len = uvsnprintf(str, TELNETD_CONF_LINELEN, pcString, vaArgP);
	    va_end(vaArgP);

		cmd_out->lines[cmd_out->line] = str;
	}
	
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
#ifdef MAINT
int cmd_clear(int argc, char *argv[]) {
    long lEEPROMRetStatus;
    lEEPROMRetStatus = SoftEEPROMClear();
    
    if(lEEPROMRetStatus != 0) {
        cmd_print("\rAn error occurred during a soft EEPROM clear operation");
        return output_error(lEEPROMRetStatus);
    }
    return(0);
}
#endif

/* This function prints out some module stats
*/
int cmd_stats(int argc, char *argv[]) {
	struct uart_info *uart_config;
	struct crc_info *crc_config;
	struct relay_info *relay_config;
	char buf[3];
	
	for(size_t i = MODULE1; i <= MODULE4; i++)	{
		if(module_exists(i))	{
			switch	(module_profile_id(i)) {
				case PROFILE_UART: {
					uart_config = get_uart_profile(i);
					cmd_print("\r\nmodule id: %d base: %X port: %d rcv: %d sent: %d err: %d lost: %d buf: %d profile: ", i, uart_config->base, uart_config->port,uart_config->recv, uart_config->sent, uart_config->err, uart_config->lost,uart_config->buf_size * UART_QUEUE_SIZE);
					usnprintf((char *)&buf, 3, "%d",i+1);
					argc=2;
					argv[1]=(char *)&buf;
					read_uartmode(argc,argv);
					break;}
					case PROFILE_CRC: {
						crc_config = get_crc_profile(i);
						cmd_print("\r\nmodule id: %d profile: %s crc: %X crc2: %X", i, "crc error", crc_config->crc,crc_config->crc2);
						
						break;}
					case PROFILE_RELAY: {
						relay_config = get_relay_profile(i);
						cmd_print("\r\nmodule id: %d profile: %s start_value: %d negation: %d convert: %d", i, "relay", relay_config->start_value, relay_config->negation, relay_config->convert);
						break;
					}
			};
		} else {
			cmd_print("\r\nmodule id: %d not available", i);
		}
	}
	return(0);
}

#ifdef MAINT
int cmd_relay(int argc, char *argv[]) {
	unsigned char pin,value;
	unsigned short module;
	
	module = MODULE1;
	if(!(module_exists(module) && (module_profile_id(module) == PROFILE_RELAY))) {
		cmd_print("No relay available.");
		return(0);
	}

	if (argc == 2) {
		pin = ustrtoul(argv[1],NULL,0);
		cmd_print("\r\nrelay module: %d pin: %d value: %d",module,pin, relay_read(module,pin));

	} else if (argc == 3) {
		pin = ustrtoul(argv[1],NULL,0);
		value = ustrtoul(argv[2],NULL,16);
		relay_write(module,pin,value);
	}
	return(0);
}

int cmd_module(int argc, char *argv[]) {
	struct module_info *header;
	
	header = (struct module_info *)pvPortMalloc(sizeof(struct module_info));
	
	if (argc < 2) {
		cmd_print("\r\ni2c exists: %d",MODEE_exists(SLAVE_ADDRESS_MODULE1));
	} else {
		if(ustrncmp(argv[1],"write",5) == 0) {
			header->magic   = 0x3A;
			header->vendor  = 0x01;
			header->product = 0x01;
			header->version = 0x01;
			header->profile = ustrtoul(argv[2],NULL,16);
			header->modres  = ustrtoul(argv[3],NULL,16);
			header->dummy2  = 0;
			header->crc     = crcSlow((unsigned char *)header, sizeof(struct module_info)-sizeof(header->crc));
			
			MODEE_write(SLAVE_ADDRESS_MODULE1,(unsigned char *) header, sizeof(struct module_info), 0);
			vTaskDelay(100 / portTICK_RATE_MS); // there must be a delay after write or avoid MODEE_read()
		}
		MODEE_read(SLAVE_ADDRESS_MODULE1, (unsigned char *) header, sizeof(struct module_info), 0);
		cmd_print("\r\ni2c module magic: %X vendor: %X product: %X version: %X profile: %X modres: %X", header->magic, header->vendor, header->product, header->version, header->profile, header->modres);


	}

	vPortFree(header);
	return(0);
}
#endif

/*
 Implements the ip address command.

 This function implements the "ipaddr" (ip address) set/display command.  

*/
int cmd_static_ipaddr(int argc, char *argv[]) {
	tBoolean found;
    long lEEPROMRetStatus;
    unsigned short usdata,usdata2;
	char *param;

	if (argc < 2) {
	    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPADDR_LOW_ID, &usdata, &found);
		
	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\r\nAn error occurred during a soft EEPROM read operation");
	        return output_error(lEEPROMRetStatus);
	    }
		if(found) {
		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPADDR_HIGH_ID, &usdata2, &found);
	        cmd_print("\r\nip addr: %d.%d.%d.%d", ((usdata2 >> 8) & 0xff),((usdata2 >> 0) & 0xff),((usdata >> 8) & 0xff),((usdata >> 0) & 0xff));

		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPMASK_LOW_ID, &usdata, &found);
		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPMASK_HIGH_ID, &usdata2, &found);
	        cmd_print("\r\nmask   : %d.%d.%d.%d", ((usdata2 >> 8) & 0xff),((usdata2 >> 0) & 0xff),((usdata >> 8) & 0xff),((usdata >> 0) & 0xff));

		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPGW_LOW_ID, &usdata, &found);
		    lEEPROMRetStatus = SoftEEPROMRead(STATIC_IPGW_HIGH_ID, &usdata2, &found);
	        cmd_print("\r\ngateway: %d.%d.%d.%d", ((usdata2 >> 8) & 0xff),((usdata2 >> 0) & 0xff),((usdata >> 8) & 0xff),((usdata >> 0) & 0xff));
			return(0);
		}
		cmd_print("\r\nNo static ip address stored.");
	} else if (argc == 5){
		param = argv[2];
	    usdata2 = ustrtoul(param,&param,0) << 8;
	    usdata2 |= ustrtoul(param+1,&param,0);
	    lEEPROMRetStatus = SoftEEPROMWrite(STATIC_IPADDR_HIGH_ID, usdata2);

	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\rAn error occurred during a soft EEPROM write operation");
	        return output_error(lEEPROMRetStatus);
	    }
	    usdata2 = ustrtoul(param+1,&param,0) << 8;
	    usdata2 |= ustrtoul(param+1,&param,0);
	    SoftEEPROMWrite(STATIC_IPADDR_LOW_ID, usdata2);

		param=argv[3];
	    usdata2 = ustrtoul(param+1,&param,0) << 8;
	    usdata2 |= ustrtoul(param+1,&param,0);
	    SoftEEPROMWrite(STATIC_IPMASK_HIGH_ID, usdata2);

	    usdata2 = ustrtoul(param+1,&param,0) << 8;
	    usdata2 |= ustrtoul(param+1,&param,0);
	    SoftEEPROMWrite(STATIC_IPMASK_LOW_ID, usdata2);

		param=argv[4];
	    usdata2 = ustrtoul(param+1,&param,0) << 8;
	    usdata2 |= ustrtoul(param+1,&param,0);
	    SoftEEPROMWrite(STATIC_IPGW_HIGH_ID, usdata2);

	    usdata2 = ustrtoul(param+1,&param,0) << 8;
	    usdata2 |= ustrtoul(param+1,&param,0);
	    SoftEEPROMWrite(STATIC_IPGW_LOW_ID, usdata2);

	} else {
        cmd_print("Wrong number of arguments\r\n");
        return(ERROR_UNHANDLED);
	}
	return(0);
}

int cmd_ipmode(int argc, char *argv[]) {
	tBoolean found;
    long lEEPROMRetStatus;
    unsigned short usdata;
	struct netif *lwip_netif;
	
	if (argc < 2) {
	    lEEPROMRetStatus = SoftEEPROMRead(IPMODE_ID, &usdata, &found);
		
	    if(lEEPROMRetStatus != 0) {
	        cmd_print("\rAn error occurred during a soft EEPROM read operation");
	        return output_error(lEEPROMRetStatus);
	    }
		usdata = found ? usdata : 1;
		cmd_print("ip mode: %s ", usdata == 1 ? "dhcp" : "static");
		lwip_netif = get_actual_netif();
		if(netif_is_up(lwip_netif)) {
			cmd_print(" ip addr: %d.%d.%d.%d mask: %d.%d.%d.%d gateway: %d.%d.%d.%d \r\n", ip4_addr1(&lwip_netif->ip_addr), ip4_addr2(&lwip_netif->ip_addr), ip4_addr3(&lwip_netif->ip_addr), ip4_addr4(&lwip_netif->ip_addr), ip4_addr1(&lwip_netif->netmask), ip4_addr2(&lwip_netif->netmask), ip4_addr3(&lwip_netif->netmask), ip4_addr4(&lwip_netif->netmask), ip4_addr1(&lwip_netif->gw), ip4_addr2(&lwip_netif->gw), ip4_addr3(&lwip_netif->gw), ip4_addr4(&lwip_netif->gw));
		} else {
			cmd_print(" network link is down.\r\n");
		}
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
		if(usdata == 0) {
			cmd_static_ipaddr(argc,argv);
		}
	}
	return(0);
}


int read_uartmode(int argc, char *argv[]) {
    unsigned short uart_base,uart_len,uart_stop;
	unsigned long ul_base,uart_speed,uart_config;
	char uart_parity;
	
	if (!argc == 2) {
        cmd_print("Wrong number of arguments\r\n");
        return(ERROR_UNHANDLED);
	}

	uart_base = ustrtoul(argv[1],NULL,0);

	if(uart_base > 0 && !module_exists(uart_base-1)) {
		cmd_print("\r\nUart not initialised. Module %d doesn't exists.", uart_base);
		return(0);
	}
	
	switch(uart_base) {
		case 1: {ul_base=UART1_BASE; break;}
		case 2: {ul_base=UART2_BASE;break;}
		default: {ul_base=UART0_BASE;}
	}
	
	UARTConfigGetExpClk(ul_base,SysCtlClockGet(), &uart_speed, &uart_config);
	
	switch(uart_config & UART_CONFIG_WLEN_MASK) {
		case UART_CONFIG_WLEN_8: { uart_len=8; break;}
		case UART_CONFIG_WLEN_7: { uart_len=7; break;}
		case UART_CONFIG_WLEN_6: { uart_len=6; break;}
		case UART_CONFIG_WLEN_5: { uart_len=5; break;}
		default: {uart_len=0;}
    }
	switch(uart_config & UART_CONFIG_STOP_MASK) {
		case UART_CONFIG_STOP_ONE: { uart_stop=1; break;}
		case UART_CONFIG_STOP_TWO: { uart_stop=2; break;}
		default: {uart_stop=0;}
	}
	
	switch(uart_config & UART_CONFIG_PAR_MASK) {
		case UART_CONFIG_PAR_NONE: { uart_parity='N'; break;}
		case UART_CONFIG_PAR_EVEN: { uart_parity='E'; break;}
		case UART_CONFIG_PAR_ODD: { uart_parity='O'; break;}
		case UART_CONFIG_PAR_ONE: { uart_parity='1'; break;}
		case UART_CONFIG_PAR_ZERO: { uart_parity='0'; break;}
		default: {uart_parity='X';}
	}

    // uart baud rate doesn't divide perfectly into whatever your system clock is set to
	cmd_print("uart%d speed: %d config: %d %c %d", uart_base, uart_speed,  uart_len , uart_parity, uart_stop );
	return (0);
}


void save_uart_config(unsigned char slave_address,unsigned long uart_speed, unsigned short config, unsigned short uart_base, unsigned short buf_size) {
	struct uart_profile *profile;
	
	if(MODEE_exists(slave_address)) {
		profile = (struct uart_profile *)pvPortMalloc(sizeof(struct uart_profile));
		profile->profile  = PROFILE_UART;
		profile->baud   = uart_speed;
		profile->config = config;
		profile->buf_size = buf_size;
		MODEE_write(slave_address,(unsigned char *) profile, sizeof(struct uart_profile), sizeof(struct module_info)); // write after header
		vPortFree(profile);
	} else {
		cmd_print("\r\n Module %d doesn't exists.",uart_base);
	}
}

/*
 This function implements the uart set/display command.  

*/
int cmd_uartmode(int argc, char *argv[]) {

    long lEEPROMRetStatus;
    unsigned short uart_base,data,buf_size;
	unsigned long  ul_base,uart_speed;
	
	if (!(argc == 2 || argc == 6 || argc == 7 )) {
        cmd_print("Wrong number of arguments\r\n");
        return(ERROR_UNHANDLED);
	}

	if (argc == 2) {
		read_uartmode(argc,argv);
	} else {
		uart_base = ustrtoul(argv[1],NULL,0);
		if(uart_base > 2) {
			cmd_print("Wrong uart device: UART%d. Valid devices are UART0...UART2\r\n", uart_base);
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
		
		if(uart_base == 0) {
			lEEPROMRetStatus=SoftEEPROMWrite(UART0_SPEED_HIGH_ID, (unsigned short)(uart_speed >> 16));
		    if(lEEPROMRetStatus != 0) {
				cmd_print("\r\nAn error occurred during a soft EEPROM write operation");
				return output_error(lEEPROMRetStatus);
			}

			lEEPROMRetStatus=SoftEEPROMWrite(UART0_SPEED_LOW_ID, (unsigned short)uart_speed );
			lEEPROMRetStatus=SoftEEPROMWrite(UART0_CONFIG_ID, data);
		} else {
			if(!(module_exists(uart_base-1) && module_profile_id(uart_base-1) == PROFILE_UART)) {
				cmd_print("\r\nUart not initialised. Module %d doesn't exists or has the wrong type.", uart_base);
				return(0);
			}
			
			if(argc == 7) {
				buf_size = ustrtoul(argv[6],NULL,0);
			} else {
				buf_size = 1;
			}
			switch(uart_base) {
				case 1: { save_uart_config(SLAVE_ADDRESS_MODULE1,uart_speed,data,uart_base,buf_size); break;}
				case 2: { save_uart_config(SLAVE_ADDRESS_MODULE2,uart_speed,data,uart_base,buf_size); break;}
				case 3: { save_uart_config(SLAVE_ADDRESS_MODULE3,uart_speed,data,uart_base,buf_size); break;}
				case 4: { save_uart_config(SLAVE_ADDRESS_MODULE4,uart_speed,data,uart_base,buf_size); break;}
			}
			
		}
		
		switch(uart_base) {
			case 1: {ul_base=UART1_BASE; break;}
			case 2: {ul_base=UART2_BASE;break;}
			default: {ul_base=UART0_BASE;}
		}

	    UARTConfigSetExpClk(ul_base, SysCtlClockGet(), uart_speed, data);

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
    cmdline_entry *pEntry;

	cmd_print("\r\nAvailable commands\r\n------------------\r\n");
    // Point at the beginning of the command table.
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->cmd)   {
        cmd_print("%s%s\r\n", pEntry->cmd, pEntry->help);
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

#ifdef MAINT
int cmd_macaddr (int argc, char *argv[]) {
	unsigned long user0,user1;
	char *param;
		
	if (!(argc == 1 || argc == 2)) {
        cmd_print("Wrong number of arguments\r\n");
        return(ERROR_UNHANDLED);
	}

	if (argc == 1) {
		FlashUserGet(&user0, &user1);
		if ((user0 == 0xffffffff) || (user1 == 0xffffffff)) {
			// Mac address range of the NUX family
			user0 = 0x005550A4;
			user1 = 0x00000000;
		}
		cmd_print("\r\nmac addr: %x:%x:%x:%x:%x:%x", ((user0 >> 0) & 0xff), ((user0 >> 8) & 0xff),((user0 >> 16) & 0xff), ((user1 >> 0) & 0xff), ((user1 >> 8) & 0xff),((user1 >> 16) & 0xff));
	} else {
		FlashUserGet(&user0, &user1);
		//Remember once committed the values in the user0 and user1 registers cannot be changed or reset. 
		if ((user0 == 0xffffffff) || (user1 == 0xffffffff)) {
			param=argv[1];
			user0=0;
			user1=0;
			user0 |= ((ustrtoul(param,&param,16) <<0 ) & 0x000000ff);
			user0 |= ((ustrtoul(param+1,&param,16) <<8 ) & 0x0000ff00);
			user0 |= ((ustrtoul(param+1,&param,16) <<16 ) & 0x00ff0000);
			user1 |= ((ustrtoul(param+1,&param,16) <<0 ) & 0x000000ff);
			user1 |= ((ustrtoul(param+1,&param,16) <<8 ) & 0x0000ff00);
			user1 |= ((ustrtoul(param+1,NULL,16) <<16 ) & 0x00ff0000);

			FlashUserSet(user0,user1);
			FlashUserSave();
		} else {
			cmd_print("You can set macaddr only one time.");
		}
	}
	return (0);
}
#endif

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
cmdline_entry g_sCmdTable[] = {
    { "help",   cmd_help,      " : Display list of commands" },
#ifdef MAINT
    { "clear",  cmd_clear,  "    : Reset soft EEPROM - Usage: clear" },
	{ "macaddr", cmd_macaddr, ": set/display mac address - macaddr <xx:xx:xx:xx:xx:xx>"},
    { "module", cmd_module, ": set/display eeprom data - Usage: module [read|write]" },
    { "relay", cmd_relay, ": set/display relay pins - Usage: relay <pin> <value>" },
#endif
    { "restart",  cmd_restart,  "    : Restart software  - Usage: restart" },
    { "ipmode", cmd_ipmode, ": set/display ipmode - Usage: ipmode [dhcp|static] [addr mask gateway]" },
    { "uart",   cmd_uartmode, ": set/display uart - uart  <id> <speed> <len> <stop> <parity> [buf]" },
    { "stats", cmd_stats, ": displays some statistics" },
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
        cmd_status = cmdline_process(cmd_buf);

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


