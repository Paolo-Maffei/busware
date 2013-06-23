
#ifndef SERIALLINK_H
#define SERIALLINK_H

#include "ringbuf.h"

#define BUFSIZE 128
#define CC1101_MAX_PAYLOAD 60

#define SL_OK    0
#define SL_ERROR 1
#define SL_NONE  2

struct apkt {
  uint8_t  seq;
  uint8_t  data[CC1101_MAX_PAYLOAD+1];
  uint8_t  len;
  uint16_t timeout;
};

void slink_init(uint16_t channel);
int slink_put(uint8_t c);
int slink_get(void);
int slink_size(void);
int slink_elements(void);
int slink_avail(void);
void slink_send(struct apkt *pkt);
uint8_t slink_recv(struct apkt *pkt);
void slink_done(void);

#endif
