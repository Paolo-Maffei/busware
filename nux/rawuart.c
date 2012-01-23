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
#include "queue.h"
#include "semphr.h"

#include "lwip/tcp.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/uart.h"

#ifdef LWIP_DEBUG
#define LWIP_DEBUGAPPS LWIPDebug
#else
#define LWIP_DEBUGAPPS while(0)((int (*)(char *, ...))0)
#endif

extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);
extern void read_iac(char * dataptr, int len,struct tcp_pcb *tpcb);

enum uart_states {
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

#define DATA_BUF_SIZE						( 256 )

int instance = 0;

struct uart_state {
	u8_t state;
	u16_t left;
	char data[DATA_BUF_SIZE];
};

void uart_error(void *arg, err_t err) {
	struct uart_state *es;

	instance=0;
	LWIP_UNUSED_ARG(err);

	es = (struct uart_state *)arg;
	if (es != NULL) {
		vPortFree(es);
	}
}

void uart_close(struct tcp_pcb *tpcb, struct uart_state *es) {
	instance=0;
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);
	tcp_sent(tpcb, NULL);
	if (es != NULL) {
		vPortFree(es);
	}

	tcp_close(tpcb);
}

static void send_data(struct tcp_pcb *pcb, struct uart_state *es) {
	err_t err;
	u16_t len,i;
	char *data;
	extern unsigned int stats_uart1_rcv;
	
	if(es->left > 0) {
		data = es->data + DATA_BUF_SIZE - es->left;
		len  = es->left;
	} else {
		len = tcp_sndbuf(pcb);

	    if(len > (2*pcb->mss)) {
			len = 2*pcb->mss;
	    }
		if( len > DATA_BUF_SIZE) {
			len = DATA_BUF_SIZE;
		}

		i=0;
		data=es->data;
		while (UARTCharsAvail(UART1_BASE) && (i<len)) {
			*data++ = UARTCharGet(UART1_BASE);
			stats_uart1_rcv++;
			i++;
		}
		if(i == 0) {
			return;
		} else {
			len = i;
		}
		
		es->left = len;
		data = es->data;
	}
	
    do {
      err = tcp_write(pcb, data, len,  1);
      if (err == ERR_MEM) {
        len /= 2;
      }
    } while (err == ERR_MEM && len > 1);

	es->left -= len;
    if (err == ERR_OK) {
    	tcp_output(pcb);
	}
}

static err_t uart_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
  struct uart_state *hs;

  LWIP_UNUSED_ARG(len);

  if(!arg) {
    return ERR_OK;
  }

  hs = arg;

  /* Temporarily disable send notifications */
  tcp_sent(pcb, NULL);

  send_data(pcb, hs);

  /* Reenable notifications. */
  tcp_sent(pcb, uart_sent);

  return ERR_OK;
}

err_t uart_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
 	struct uart_state *es;
	char *data;
	extern unsigned int stats_uart1_sent;
			
  	LWIP_ASSERT("arg != NULL",arg != NULL);
  	es = (struct uart_state *)arg;

	if (p == NULL)   {
		err = ERR_OK;
	} else if(err != ERR_OK)  {
	/* cleanup, for unkown reason */
		if (p != NULL) {
			pbuf_free(p);
		}
	} else if(es->state == ES_ACCEPTED) {
		tcp_recved(tpcb, p->tot_len);

		data = (char *)p->payload;
		read_iac(data, p->len,tpcb);
		es->state = ES_RECEIVED;
		pbuf_free(p);
		tcp_output(tpcb);
	} else  if (es->state == ES_RECEIVED) {
		tcp_recved(tpcb, p->tot_len);
		do {
			data = (char *)p->payload;
			UARTSend(UART1_BASE,data,p->len);
			stats_uart1_sent += p->len;
		} while((p = p->next));
		
		pbuf_free(p);
	}
	return err;
}

static err_t uart_poll(void *arg, struct tcp_pcb *pcb){
	struct uart_state *hs;
	
	hs = arg;

	if ((hs == NULL) && (pcb->state == ESTABLISHED)) {
		tcp_abort(pcb);
		return ERR_ABRT;
	} else if(pcb->state == CLOSE_WAIT){
		uart_close(pcb,hs);
		return ERR_OK;
	}

	send_data(pcb, hs);
	
	return ERR_OK;
}

err_t uart_accept(void *arg, struct tcp_pcb *tpcb, err_t err) {
    struct uart_state *es;
	
	LWIP_UNUSED_ARG(arg);

/* commonly observed practive to call tcp_setprio(), why? */
	tcp_setprio(tpcb, TCP_PRIO_MIN);

	if(instance > 0) {
		return ERR_BUF;
	}
	instance=1;
	es = (struct uart_state *)pvPortMalloc(sizeof(struct uart_state));
  	if (es != NULL)   {
		es->state = ES_ACCEPTED;
		es->left  = 0;
		/* pass newly allocated es to our callbacks */
	    tcp_arg(tpcb, es);
		tcp_err(tpcb, uart_error);
		tcp_recv(tpcb, uart_recv);
	  	tcp_poll(tpcb, uart_poll, 1);
		tcp_sent(tpcb, uart_sent);
	} else {
		err = ERR_MEM;
	}
	
	return err;  
}

void rawuart_init(void) {
	struct tcp_pcb *pcb;
	
	pcb = tcp_new();
	if (pcb != NULL)   {
		err_t err;

		err = tcp_bind(pcb, IP_ADDR_ANY, 1234);
		if (err == ERR_OK)   {
			pcb = tcp_listen(pcb);
			tcp_accept(pcb, uart_accept);
		}
	}
}