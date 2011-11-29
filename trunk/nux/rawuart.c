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


extern void LWIPDebug(const char *pcString, ...);

void readuart_thread(void *pvParameters) {
	struct netconn *conn, *newconn;
	err_t err;
	struct netbuf *buf;
	portCHAR data;
	void *indata;
	u16_t len;
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
		// there's a bug in version 1.3.0 so we've to upgrade before we can use timeout
		//newconn->recv_timeout = 100; // wait 100ms for data from ethernet
		
		LWIPDebug("rawuart connection accepted\r\n");
		
		for(;;) {
			len = uxQueueMessagesWaiting(xUART1Queue);
			while(len > 0) {
				if (xQueueReceive( xUART1Queue, &data, portMAX_DELAY) == pdPASS) {
					err = netconn_write(newconn, &data, sizeof(portCHAR), NETCONN_COPY);
					if(err != ERR_OK) {
						LWIPDebug("rawuart netconn_write err: %d\r\n",err);

						goto finish;
					}
				}
				len--;
			}
/*			
If timeout = 0 then it blocks until we get data
If timeout > 0 then the next netconn_write will fail due to a bug in 1.3.0
http://old.nabble.com/TCP-write-won't-work-after-netconn_recv-timeout-td25802971.html

			buf = netconn_recv(newconn);
			if (buf != NULL) {
				netbuf_data(buf, &indata, &len);
				UARTSend(UART1_BASE,indata,len);
				netbuf_delete(buf);
			} else if (netconn_err(newconn) != ERR_TIMEOUT) {
				LWIPDebug("rawuart netconn_recv err: %d\r\n",netconn_err(newconn));
				goto finish;
			}
*/
		};
		/* Close connection and discard connection identifier. */
finish:
		netconn_close(newconn);
		netconn_delete(newconn);
		LWIPDebug("rawuart connection closed\r\n");
	}
}


void rawuart_init(void) {
	if (pdPASS != xTaskCreate( readuart_thread, ( signed portCHAR * ) "READUART", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 3  , NULL )) {
		LWIPDebug("Cant create readuart!");
	}
}