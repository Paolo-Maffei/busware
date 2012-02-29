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


#include "lwip/sys.h"
#include "lwip/api.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/uart.h"

#include "modules.h"

#ifdef LWIP_DEBUG
#define LWIP_DEBUGAPPS LWIPDebug
#else
#define LWIP_DEBUGAPPS while(0)((int (*)(char *, ...))0)
#endif

extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);

void readuart_thread(void *params) {
	struct uart_info *uart;
	struct netconn *conn, *newconn;
	err_t err;
	struct netbuf *buf;
	char *inbuf, *outbuf;
	u16_t i,len;

	uart = (struct uart_info*)params;
	
	LWIP_DEBUGAPPS("Listen on port %d", uart->port);
	/* Create a new connection identifier. */
	conn = netconn_new(NETCONN_TCP);

	/* Bind connection to well known port number 7. */
	netconn_bind(conn, NULL, uart->port);

	/* Tell connection to go into listening mode. */
	netconn_listen_with_backlog(conn,1); // resolve uart access if you need more then 1 connection at the time

	while (1) {
		/* Grab new connection. */
		newconn = netconn_accept(conn);
		newconn->recv_timeout = 75; // wait 75ms for data from ethernet
		outbuf = (portCHAR *)pvPortMalloc(UART_QUEUE_SIZE);
		
		LWIP_DEBUGAPPS("rawuart connection accepted\r\n");
	
		for(;;) {
			len=0;
            len = uxQueueMessagesWaiting(uart->queue);
            if(len > 0) {
                for(i=0; i < len;i++) {
                        xQueueReceive( uart->queue, outbuf+i, portMAX_DELAY);
                }
				err = netconn_write(newconn, outbuf, len, NETCONN_COPY);
				if(err != ERR_OK) {
					LWIP_DEBUGAPPS("rawuart netconn_write err: %d\r\n",err);
					goto finish;
				}
			}
			
			buf = netconn_recv(newconn);
			if (buf != NULL) {
				do {
					netbuf_data(buf, (void *)&inbuf, &len);
					UARTSend(uart->base,inbuf,len); // this is blocking until all characters have been sent, consider buffering if it's too slow
					uart->sent += len;
				} while (netbuf_next(buf) != -1); // read all data
				netbuf_delete(buf);
			} else if (netconn_err(newconn) != ERR_TIMEOUT) {
				LWIP_DEBUGAPPS("rawuart netconn_recv err: %d\r\n",netconn_err(newconn));
				goto finish;
			}
		};
		/* Close connection and discard connection identifier. */
finish:
		vPortFree(outbuf);
		netconn_close(newconn);
		netconn_delete(newconn);
		LWIP_DEBUGAPPS("rawuart connection closed\r\n");
	}
}


void rawuart_init(u16_t port, struct uart_info *uart_config) {
	if (pdPASS != xTaskCreate( readuart_thread, ( signed portCHAR * ) "UART", configMINIMAL_STACK_SIZE, uart_config , tskIDLE_PRIORITY + 3  , NULL )) {
		LWIP_DEBUGAPPS("Cant create task uart!\r\n");
	}
}