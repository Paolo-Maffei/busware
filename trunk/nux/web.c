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

#include "utils/vstdlib.h"
#include "modules.h"
#include "relay.h"

#ifdef LWIP_DEBUG
#define LWIP_DEBUGAPPS LWIPDebug
#else
#define LWIP_DEBUGAPPS while(0)((int (*)(char *, ...))0)
#endif

#ifndef true
#define true ((u8_t)1)
#endif

#ifndef false
#define false ((u8_t)0)
#endif

#define NOT_SUPPORTED  501
#define OBJECT_NOT_FOUND 401
#define HTTP_OK 200
#define OK_NO_CONTENT 204

const char * const HTTP_MSG_OK =  "HTTP/1.1 200 OK\r\n";
const char * const HTTP_NOT_FOUND = "HTTP/1.1 404 File not found\r\n";
const char * const HTTP_NOT_IMPLEMENTED = "HTTP/1.1 501 Not Implemented\r\n";
const char * const HTTP_SERVER_NAME = "Server: nux/0.1 (http://www.busware.de/)\r\n";
const char * const NOT_FOUND_DATA = "{error: 404, text: 'Object not found'}\r\n";
const char * const CONTENT_TYPE_JSON = "Content-type: application/json\r\n";
const char * const HTTP_NO_CONTENT = "HTTP/1.1 204 No Content\r\n";
const char * const EMPTY_LINE = "\r\n";


#define BUF_SIZE 256
#define NUM_FILE_HDR_STRINGS 3

struct web_state {
	char *buf;        /* Read buffer. */
	int buf_len;      /* Size of file read buffer, buf. */
	int buf_pos;
	const char *hdrs[NUM_FILE_HDR_STRINGS]; /* HTTP headers to be sent. */
	u16_t hdr_index;   /* The index of the hdr string currently being sent. */
	char content_len_hdr[25];
};


void web_close(struct tcp_pcb *tpcb, struct web_state *es) {
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	if (es != NULL) {
		if(es->buf != NULL) {
			vPortFree(es->buf);
		}
		vPortFree(es);
	}

	tcp_close(tpcb);
}

static void send_data(struct tcp_pcb *pcb, struct web_state *es) {
	err_t err;
	u16_t len,hdrlen;
	u8_t has_data;

	len = tcp_sndbuf(pcb);
	has_data = false;
  /* Do we have any more header data to send for this file? */
	if(es->hdr_index < NUM_FILE_HDR_STRINGS)  { // print out constant header
		while(es->hdr_index < NUM_FILE_HDR_STRINGS) {
			hdrlen = ustrlen(es->hdrs[es->hdr_index]);
			if(hdrlen <= len) {
				err = tcp_write(pcb, (const void *)(es->hdrs[es->hdr_index]), hdrlen, 0);
				if(err == ERR_MEM) {
					break;
				}
				has_data=true;
				es->hdr_index++;
				len -=hdrlen;
			} else {
				break;
			}
		}
  	}

	if(es->hdr_index == NUM_FILE_HDR_STRINGS) {
		hdrlen = ustrlen(es->content_len_hdr); // print out content-length
		if(hdrlen > 0) {
			err = tcp_write(pcb, (const void *)(es->content_len_hdr), hdrlen, 1);
			if(err == ERR_OK) {
				has_data=true;
				len -= hdrlen;
				es->content_len_hdr[0]='\0';
				hdrlen = es->buf_len - es->buf_pos;
			} else {
				hdrlen=0;
			}
		} else {
			hdrlen = es->buf_len - es->buf_pos;
		}

		if(hdrlen > 0) { // print out buffer
			if(hdrlen > len) {
				hdrlen=len;
			}
			do {
		          err = tcp_write(pcb, (const void *)(es->buf + es->buf_pos), hdrlen, 1);
		          if (err == ERR_MEM) {
		            hdrlen /= 2;
		          }
		          else if (err == ERR_OK) {
		            /* Remember that we added some more data to be transmitted. */
		            has_data = true;
					es->buf_pos += hdrlen;
		          }
		    } while ((err == ERR_MEM) && hdrlen);
		}
	}

	if(has_data) {
		tcp_output(pcb);
		LWIP_DEBUGAPPS("Send data.");
	} else {
		web_close(pcb,es);
	}
	
}

static err_t web_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
  struct web_state *hs;

  LWIP_UNUSED_ARG(len);

  if(!arg) {
    return ERR_OK;
  }

  hs = arg;

  /* Temporarily disable send notifications */
  tcp_sent(pcb, NULL);

  send_data(pcb, hs);

  /* Reenable notifications. */
  tcp_sent(pcb, web_sent);

  return ERR_OK;
}

void add_header(unsigned int http_code, struct web_state *es) {
	es->hdrs[1] = HTTP_SERVER_NAME;

	switch (http_code) {
		case NOT_SUPPORTED: {
			es->hdrs[0] = HTTP_NOT_IMPLEMENTED;
			es->hdrs[2] = CONTENT_TYPE_JSON;
			break;
		}
		case OBJECT_NOT_FOUND: {
			es->hdrs[0] = HTTP_NOT_FOUND;
			es->hdrs[2] = CONTENT_TYPE_JSON;
			break;
		}
		case OK_NO_CONTENT: {
			es->hdrs[0] = HTTP_NO_CONTENT;
			es->hdrs[2] = EMPTY_LINE;
			break;
		}
		case HTTP_OK: {
			es->hdrs[0] = HTTP_MSG_OK;
			es->hdrs[2] = CONTENT_TYPE_JSON;
		}
	}
	es->hdr_index=0;
}

void add_content(struct web_state *es,const char *pcString, ...) {
    va_list vaArgP;

    va_start(vaArgP, pcString);
    es->buf_len += uvsnprintf(es->buf+es->buf_len, BUF_SIZE - es->buf_len, pcString, vaArgP);
    va_end(vaArgP);
	usnprintf(es->content_len_hdr, 25, "Content-Length: %d\r\n\r\n", es->buf_len);
}


err_t web_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	char *data,*uri;
 	struct web_state *es;
	unsigned int i,module,pin,val;
	u8_t is_get = false;
	u8_t not_supported = false;
	u8_t no_object = false;
	
  	LWIP_ASSERT("arg != NULL",arg != NULL);
  	es = (struct web_state *)arg;

	if (p == NULL)   {
		err = ERR_OK;
	} else if(err != ERR_OK)  {
	/* cleanup, for unkown reason */
		if (p != NULL) {
			pbuf_free(p);
		}
		err = err;
	} else {
		tcp_recved(tpcb, p->tot_len);

		data = p->payload;
		uri = &data[4];
		if (ustrncmp(data, "GET ", 4) == 0) {
			is_get = true;
		} if (!(ustrncmp(data, "POST ", 5) == 0)) {
			not_supported=true;
		}
		
		// for the moment we ignore data not in the first pbuf block
        for(i = 4; i < (p->len - 5); i++) {
          if ((data[i] == ' ') && (data[i + 1] == 'H') &&
              (data[i + 2] == 'T') && (data[i + 3] == 'T') &&
              (data[i + 4] == 'P')) {
            data[i] = 0;
            break;
          }
        }
        if((i == (p->len - 5))) {
          /* We failed to find " HTTP" in the request so assume it is invalid */
          LWIP_DEBUGAPPS("Invalid request. Closing.");
          pbuf_free(p);
          web_close(tpcb, es);
          return(ERR_OK);
        }
		if(not_supported) {
			add_header(NOT_SUPPORTED,es);
		}
		LWIP_DEBUGAPPS("requested uri: %s", uri);
		data=ustrstr(uri,"/");
		if(ustrncmp(data,"/relay",6) == 0) {
			data=ustrstr(data+1,"/");
			if(ustrlen(data) > 1) {
				module=ustrtoul(data+1,&data,0);
				module--;
				if(module_exists(module) && module_profile_id(module) == PROFILE_RELAY) {
					LWIP_DEBUGAPPS("mod: relay");

					if(ustrlen(data) > 1) {
						pin=ustrtoul(data+1,&data,0);
						if(pin_exists(module,pin-1)) {
							if(is_get) {
								add_header(HTTP_OK,es);
								add_content(es,"{ pin%d: %X}\r\n", pin, relay_read(module,pin-1));
							} else {
								if(ustrlen(data) > 1) {
									val=ustrtoul(data+1,&data,0);
									relay_write(module,pin-1,val);
									add_header(OK_NO_CONTENT,es);
								} else {
									no_object=true;
								}
							}
						} else {
							no_object=true;
						}
					} else {
						if(is_get) {
							add_header(HTTP_OK,es);
							add_content(es,"{ pin1: %X , pin2: %X}\r\n", relay_read(module,0), relay_read(module,1));
						}
					}
				} else {
					LWIP_DEBUGAPPS("Invalid resource: module %d",module);
					no_object=true;
				}
			} else {
				LWIP_DEBUGAPPS("Invalid resource: module %s",data);
				no_object = true;
			}
		} else {
			LWIP_DEBUGAPPS("Invalid resource: %s",uri);
			no_object=true;
		} 
		
		if(no_object) {
			add_header(OBJECT_NOT_FOUND,es);
			add_content(es,NOT_FOUND_DATA);
		}
		
		pbuf_free(p);
		/* Start sending the headers and data. */
        send_data(tpcb, es);
	}
	
	if ((err == ERR_OK) && (p == NULL)) {
    	web_close(tpcb, es);
  	}
	return err;
}

void web_error(void *arg, err_t err) {
  struct web_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct web_state *)arg;
  if (es != NULL) {
	if(es->buf != NULL) {
		vPortFree(es->buf);
	}
    vPortFree(es);
  }
}


static err_t web_poll(void *arg, struct tcp_pcb *pcb){
        struct web_state *hs;
        
        hs = arg;

        if ((hs == NULL) && (pcb->state == ESTABLISHED)) {
                tcp_abort(pcb);
                return ERR_ABRT;
        } else if(pcb->state == CLOSE_WAIT){
                web_close(pcb,hs);
                tcp_abort(pcb);
                return ERR_ABRT;
        }

        send_data(pcb, hs);
        return ERR_OK;
}


err_t web_accept(void *arg, struct tcp_pcb *tpcb, err_t err) {
    struct web_state *es;
	LWIP_UNUSED_ARG(arg);

/* commonly observed practive to call tcp_setprio(), why? */
	tcp_setprio(tpcb, TCP_PRIO_MIN);
	
	es = (struct web_state *)pvPortMalloc(sizeof(struct web_state));
  	if (es != NULL)   {
	  	/* We don't have a send buffer so allocate one up to 2mss bytes long. */
        es->buf = pvPortMalloc(BUF_SIZE);
    	if (es->buf == NULL) {
			return ERR_MEM;
		}
		es->buf_pos = 0;
		es->buf_len = 0;
		*(es->buf)  = '\0';
		es->hdr_index = NUM_FILE_HDR_STRINGS;
		es->content_len_hdr[0]='\0';
		
		/* pass newly allocated es to our callbacks */
	    tcp_arg(tpcb, es);
		tcp_err(tpcb, web_error);
		tcp_recv(tpcb, web_recv);
		tcp_sent(tpcb, web_sent);
		tcp_poll(tpcb, web_poll,5);
	} else {
		err = ERR_MEM;
	}
	
	return err;  
}

void web_init(unsigned long port) {
	struct tcp_pcb *pcb;
	
	pcb = tcp_new();
	if (pcb != NULL)   {
		err_t err;

		err = tcp_bind(pcb, IP_ADDR_ANY, port);
		if (err == ERR_OK)   {
			pcb = tcp_listen(pcb);
			tcp_accept(pcb, web_accept);
		}
	}
}