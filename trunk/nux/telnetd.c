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


#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "utils/cmdline.h"
#include "utils/vstdlib.h"
#include "console.h"


#ifdef LWIP_DEBUG
#define LWIP_DEBUGAPPS LWIPDebug
#else
#define LWIP_DEBUGAPPS while(0)((int (*)(char *, ...))0)
#endif

extern const portCHAR * const prompt;
extern const portCHAR * const welcome;

extern struct console_state *cmd_out;

const portCHAR * const UNKNOWN_COMMAND = "Unknown command\n";
const portCHAR * const TOO_MANY_ARGS ="Too many arguments for command processor!\n";

int accept;

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


static void send_data(struct tcp_pcb *pcb, struct telnet_state *es) {
	err_t err;
	u16_t len,i;
	u8_t has_data;
	
	if(cmd_out->line > -1) {
		len = tcp_sndbuf(pcb);
		has_data = 0;
		for(i=0;i<=cmd_out->line;i++) {
			if(cmd_out->lines[i] == NULL) {
				continue;
			}
			
			if(ustrlen(cmd_out->lines[i]) < len) {
				err = tcp_write(pcb, cmd_out->lines[i], ustrlen(cmd_out->lines[i]), 1);
				if(err == ERR_MEM) {
					break;
				}
				has_data = 1;
				len -= ustrlen(cmd_out->lines[i]);
			
				vPortFree(cmd_out->lines[i]);
				cmd_out->lines[i] = NULL;

			} else {
				break;
			}
		};

		if (has_data) {
			tcp_output(pcb);
		}
		if(cmd_out->lines[cmd_out->line] == NULL) {
			cmd_out->line=-1;
		}
	}

}

void telnet_error(void *arg, err_t err) {
  struct telnet_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct telnet_state *)arg;
  if (es != NULL) {
    vPortFree(es);
  }

	accept--;
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
	if(cmd_out != NULL) {
		vPortFree(cmd_out);
	}

	tcp_close(tpcb);
	accept--;
}

static err_t telnet_poll(void *arg, struct tcp_pcb *pcb){
        struct telnet_state *hs;
        
        hs = arg;

        if ((hs == NULL) && (pcb->state == ESTABLISHED)) {
                tcp_abort(pcb);
                return ERR_ABRT;
        } else if(pcb->state == CLOSE_WAIT){
                telnet_close(pcb,hs);
                tcp_abort(pcb);
                return ERR_ABRT;
        }

        send_data(pcb, hs);
        
        return ERR_OK;
}

static err_t telnet_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
  struct telnet_state *hs;

  LWIP_UNUSED_ARG(len);

  if(!arg) {
    return ERR_OK;
  }

  hs = arg;

  /* Temporarily disable send notifications */
  tcp_sent(pcb, NULL);

  send_data(pcb, hs);

  /* Reenable notifications. */
  tcp_sent(pcb, telnet_sent);

  return ERR_OK;
}

#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_WILL   2
#define STATE_WONT   3
#define STATE_DO     4
#define STATE_DONT   5
#define STATE_CLOSE  6

#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

void read_iac(char * dataptr, int len,struct tcp_pcb *tpcb) {
	char c;
	u8_t state = STATE_NORMAL;
	char line[3];
	line[0] = TELNET_IAC;

	 
	while(len > 0 ) {
		c = *dataptr;
		++dataptr;
		--len;
		switch(state) {
			case STATE_IAC:
				if(c == TELNET_IAC) {
					state = STATE_NORMAL;
				} else {
					switch(c) {
						case TELNET_WILL:
								state = STATE_WILL;
								break;
						case TELNET_WONT:
								state = STATE_WONT;
								break;
						case TELNET_DO:
								state = STATE_DO;
								break;
						case TELNET_DONT:
								state = STATE_DONT;
								break;
						default:
								state = STATE_NORMAL;
								break;
					}
				}
				break;
			case STATE_WILL:
				/* Reply with a DONT */
				line[1] = TELNET_DONT;
				line[2] = c;
				tcp_write(tpcb, line,3,1);
				state = STATE_NORMAL;
				break;

			case STATE_WONT:
				/* Reply with a DONT */
				line[1] = TELNET_DONT;
				line[2] = c;
				tcp_write(tpcb, line,3,1);
				
				state = STATE_NORMAL;
				break;
			case STATE_DO:
				/* Reply with a WONT */
				line[1] = TELNET_WONT;
				line[2] = c;
				tcp_write(tpcb, line,3,1);

				state = STATE_NORMAL;
				break;
			case STATE_DONT:
				/* Reply with a WONT */
				line[1] = TELNET_WONT;
				line[2] = c;
				tcp_write(tpcb, line,3,1);
				state = STATE_NORMAL;
				break;
			case STATE_NORMAL:
				if(c == TELNET_IAC) {
					state = STATE_IAC;
				}
				break;
			}
	}
}

err_t telnet_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	char *data;
	char *dest;

	int cmd_status;
 	struct telnet_state *es;
	struct pbuf *pb;
		
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
		tcp_recved(tpcb, p->tot_len);

		data = (char *)p->payload;
		read_iac(data, p->len,tpcb);
		es->state = ES_RECEIVED;
		es->line[0] = '\0';

		err = tcp_write(tpcb, prompt, ustrlen(prompt), 0);
		if(err == ERR_OK) {
			tcp_output(tpcb);
		}
		if (p != NULL) {
			pbuf_free(p);
		}
	} else  if (es->state == ES_RECEIVED) {
		tcp_recved(tpcb, p->tot_len);
		if(p->tot_len < TELNETD_CONF_LINELEN) {
			dest = &(es->line[0]);
			pb=p;
			do {
				data = (char *)pb->payload;
				ustrncpy(dest,data,pb->len);
				dest += pb->len;
			} while((pb = pb->next));
			*dest='\0';
		}
		
		pbuf_free(p);
		if((dest=ustrstr(es->line,"\r\n")) != NULL) {
			*dest='\0';
		}
		
        //
        // Pass the line from the user to the command processor.  It will be
        // parsed and valid commands executed.
        //
        cmd_status = cmdline_process(es->line);


        if(cmd_status == CMDLINE_BAD_CMD)  {
			cmd_print(UNKNOWN_COMMAND);
		} else if(cmd_status == CMDLINE_TOO_MANY_ARGS) {
			cmd_print(TOO_MANY_ARGS);
        }


		cmd_print(prompt);
		send_data(tpcb,es);
		es->line[0] = (char)0;
		
		if (cmd_status == CMDLINE_QUIT) {
			telnet_close(tpcb,es);
		}
	}
	return err;
}

err_t telnet_accept(void *arg, struct tcp_pcb *tpcb, err_t err) {
    struct telnet_state *es;
	
	LWIP_UNUSED_ARG(arg);

/* commonly observed practive to call tcp_setprio(), why? */
	tcp_setprio(tpcb, TCP_PRIO_MIN);
	
	es = (struct telnet_state *)pvPortMalloc(sizeof(struct telnet_state));
  	if (es != NULL && accept == 0)   {
		cmd_out = (struct console_state *)pvPortMalloc(sizeof(struct console_state));
		cmd_out->line=-1;
	
		es->state = ES_ACCEPTED;
		/* pass newly allocated es to our callbacks */
	    tcp_arg(tpcb, es);
		tcp_err(tpcb, telnet_error);
		tcp_recv(tpcb, telnet_recv);
		tcp_sent(tpcb, telnet_sent);
		tcp_poll(tpcb, telnet_poll,5);
		
		err = tcp_write(tpcb, welcome, ustrlen(welcome), 0);
		if(err == ERR_OK) {
			accept++;
			tcp_output(tpcb);
		}
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
			pcb = tcp_listen(pcb); // only 1 connection at the time, resolve extern struct console_state *cmd_out if you need more
								   // listen_with_backlog doesn't work as expected
			tcp_accept(pcb, telnet_accept);
		}
	}
}