/*****************************************************************************
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
#include "semphr.h"
#include "arch/cc.h"

#include <string.h>

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "utils/cmdline.h"
#include "utils/ustdlib.h"
#include "console.h"

enum telnet_states {
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

struct telnet_state {
  u8_t state;
  char line[TELNETD_CONF_LINELEN];
};

void print_tcp(struct console_state *hs,struct tcp_pcb *pcb) {
	int i,len;
	
	len = tcp_sndbuf(pcb);
	for(i=0;i<=hs->line;i++) {
		tcp_write(pcb, hs->lines[i], strlen(hs->lines[i]), 1);
		vPortFree(hs->lines[i]);
	}
	hs->line=-1;
}


void telnet_error(void *arg, err_t err) {
  struct telnet_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct telnet_state *)arg;
  if (es != NULL) {
    vPortFree(es);
  }
}

void telnet_close(struct tcp_pcb *tpcb, struct telnet_state *es) {
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	if (es != NULL) {
		vPortFree(es);
	}

	tcp_close(tpcb);
}


err_t telnet_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	char *cmd_buf;
	char *data;
	extern struct console_state *cmd_out;
	int len,i,cmd_status;
 	struct telnet_state *es;

	extern char prompt[];
		
  	LWIP_ASSERT("arg != NULL",arg != NULL);
  	es = (struct telnet_state *)arg;

	if (p == NULL)   {
		err = ERR_OK;
	} else if(err != ERR_OK)  {
	/* cleanup, for unkown reason */
		if (p != NULL) {
			pbuf_free(p);
		}
		err = err;
	} else if(es->state == ES_ACCEPTED) {
		es->state = ES_RECEIVED;
		es->line[0] = 0;
		cmd_buf = (char *)pvPortMalloc(TELNETD_CONF_LINELEN);
		
		len = sprintf(cmd_buf, prompt);
		err = tcp_write(tpcb, cmd_buf, len, 1);

		if(err == ERR_OK) {
			tcp_output(tpcb);
		}
		vPortFree(cmd_buf);
		if (p != NULL) {
			pbuf_free(p);
		}
	} else  if (es->state == ES_RECEIVED) {
		data = (char *)p->payload;
		len=ustrlen(es->line);

		for(i=0; i < p->len; i++) {
			es->line[len+i] = *data;
			data++;
		}
		es->line[len+p->len] = (char)0;
		
		pbuf_free(p);
		if(ustrstr(es->line,"\n") == NULL) { // not finished command line
			return ERR_OK;
		}
		len=ustrlen(es->line);
		es->line[len-2] = (char)0;
		cmd_buf = (char *)pvPortMalloc(TELNETD_CONF_LINELEN);

		cmd_out = (struct console_state *)pvPortMalloc(sizeof(struct console_state));
		cmd_out->line=-1;
        //
        // Pass the line from the user to the command processor.  It will be
        // parsed and valid commands executed.
        //
        cmd_status = CmdLineProcess(es->line);

		print_tcp(cmd_out,tpcb);
		vPortFree(cmd_out);

        if(cmd_status == CMDLINE_BAD_CMD)  {
			len = sprintf(cmd_buf, "Unknown command");
			err = tcp_write(tpcb, cmd_buf, len, 1);
		} else if(cmd_status == CMDLINE_TOO_MANY_ARGS) {
			len = sprintf(cmd_buf, "Too many arguments for command processor!");
			err = tcp_write(tpcb, cmd_buf, len, 1);
        }

		len = sprintf(cmd_buf, prompt);
		err = tcp_write(tpcb, cmd_buf, len, 1);

		if(err == ERR_OK) {
			tcp_output(tpcb);
		}
		vPortFree(cmd_buf);
		es->line[0] = (char)0;
		
		if (cmd_status == CMDLINE_QUIT) {
			telnet_close(tpcb,es);
		}
	}
	return err;
}

err_t telnet_accept(void *arg, struct tcp_pcb *tpcb, err_t err) {
	int len;
	char *cmd_buf;
    struct telnet_state *es;
	extern char welcome[];
	
	LWIP_UNUSED_ARG(arg);

/* commonly observed practive to call tcp_setprio(), why? */
	tcp_setprio(tpcb, TCP_PRIO_MIN);
	
	es = (struct telnet_state *)pvPortMalloc(sizeof(struct telnet_state));
  	if (es != NULL)   {
		es->state = ES_ACCEPTED;
		/* pass newly allocated es to our callbacks */
	    tcp_arg(tpcb, es);
		tcp_err(tpcb, telnet_error);
		tcp_recv(tpcb, telnet_recv);

		cmd_buf = (char *)pvPortMalloc(TELNETD_CONF_LINELEN);

		len = sprintf(cmd_buf, welcome);
		err = tcp_write(tpcb, cmd_buf, len, 1);
		if(err == ERR_OK) {
			tcp_output(tpcb);
		}

		vPortFree(cmd_buf);
	} else {
		err = ERR_MEM;
	}
	
	return err;  
}


void telnetd_init(void) {
	struct tcp_pcb *pcb;
	
	pcb = tcp_new();
	if (pcb != NULL)   {
		err_t err;

		err = tcp_bind(pcb, IP_ADDR_ANY, 23);
		if (err == ERR_OK)   {
			pcb = tcp_listen(pcb);
			tcp_accept(pcb, telnet_accept);
		}
	}
}