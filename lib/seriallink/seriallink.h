
#ifndef SERIALLINK_H
#define SERIALLINK_H

#include "ringbuf.h"

#define BUFSIZE 128

#ifdef RADIO_CRYPT
// this due to the block size of AES == 3*16 = 48 (minus a byte for len and seq) 
// 4*16 = 64 + lenbyte wont fit into CC1101 FIFO
#define CC1101_MAX_PAYLOAD 46
#else
#define CC1101_MAX_PAYLOAD 60
#endif

#define SL_OK    0
#define SL_ERROR 1
#define SL_NONE  2

struct __attribute__ ((__packed__)) apkt {
  uint8_t  len;      // length of data
  uint8_t  seq;      // sequence no
  uint8_t  data[64]; // this is to simplify encryption 4 block of 16 bytes ...
  uint8_t  plen;     // total lenght of transmitted packet
  uint16_t timeout;
  uint8_t  start;    // offset where data to put / take from in this structure 
  uint8_t  cseq;     // the clear sequence no as above (uncrypted)
};

void slink_init(uint16_t channel);
void slink_crypt(unsigned char *Key);
int slink_put(uint8_t c);
int slink_get(void);
int slink_size(void);
int slink_elements(void);
int slink_avail(void);
void slink_send(struct apkt *pkt);
uint8_t slink_recv(struct apkt *pkt);
void slink_done(void);

#endif
