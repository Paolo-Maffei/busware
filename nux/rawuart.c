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

#include "hw_memmap.h"
#include "hw_types.h"
#include "uart.h"

#ifdef LWIP_DEBUG
#define LWIP_DEBUGAPPS LWIPDebug
#else
#define LWIP_DEBUGAPPS while(0)((int (*)(char *, ...))0)
#endif

extern void UARTSend(unsigned long ulBase, const char *pucBuffer, unsigned short ulCount);

void readuart_thread(void *pvParameters) {
	struct netconn *conn, *newconn;
	err_t err;
	struct netbuf *buf;
	portCHAR *data;
	u16_t len;
	short i;
	extern xQueueHandle xUART1Queue;


	
	/* Create a new connection identifier. */
	conn = netconn_new(NETCONN_TCP);

	/* Bind connection to well known port number 7. */
	netconn_bind(conn, NULL, 1234);

	/* Tell connection to go into listening mode. */
	netconn_listen(conn);

	while (1) {
		/* Grab new connection. */
		newconn = netconn_accept(conn);
		newconn->recv_timeout = 100; // wait 100ms for data from ethernet
		
		LWIP_DEBUGAPPS("rawuart connection accepted\r\n");
		
		for(;;) {
			len = uxQueueMessagesWaiting(xUART1Queue);
			if( len > 0) {
				data = (portCHAR *)pvPortMalloc(len);
				for(i=0; i < len;i++) {
					xQueueReceive( xUART1Queue, data+i, portMAX_DELAY);
				}
				err = netconn_write(newconn, data, len, NETCONN_COPY);
				vPortFree(data);
				if(err != ERR_OK) {
					LWIP_DEBUGAPPS("rawuart netconn_write err: %d\r\n",err);

					goto finish;
				}
			}

			buf = netconn_recv(newconn);
			if (buf != NULL) {
				do {
					netbuf_data(buf, (void *)&data, &len);
					UARTSend(UART1_BASE,data,len);
				} while (netbuf_next(buf) > 0); // read all data
				netbuf_delete(buf);
			} else if (netconn_err(newconn) != ERR_TIMEOUT) {
				LWIP_DEBUGAPPS("rawuart netconn_recv err: %d\r\n",netconn_err(newconn));
				goto finish;
			}
		};
		/* Close connection and discard connection identifier. */
finish:
		netconn_close(newconn);
		netconn_delete(newconn);
		LWIP_DEBUGAPPS("rawuart connection closed\r\n");
	}
}


void rawuart_init(void) {
	if (pdPASS != xTaskCreate( readuart_thread, ( signed portCHAR * ) "READUART", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3  , NULL )) {
		LWIP_DEBUGAPPS("Cant create readuart!\r\n");
	}
}