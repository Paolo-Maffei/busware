
#include "board.h"

#ifdef CC1100_CS_PIN

#include <stdint.h>
#include <avr/io.h>
#include "seriallink.h"
#include "ringbuf.h"
#include "cc1101.h"
#include "util/delay.h"
#include <string.h>

#define DEBUG 1

#ifdef NO_PGM
#undef DEBUG
#endif

#if DEBUG
#include <stdio.h>
#include <avr/pgmspace.h>
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...)
#endif

static struct ringbuf rxbuf;
static struct ringbuf txbuf;
static uint8_t RXBUF[BUFSIZE];
static uint8_t TXBUF[BUFSIZE];

static uint8_t rxseq;
static uint8_t txseq;
static uint8_t ccinitialized;

#ifdef RADIO_CRYPT
#include "aes.h"

#define AES_KEY_SIZE	128

unsigned char *AES_Key;
aes128_ctx_t ctx;

static uint8_t do_crypt = 0;
#endif

static struct apkt txpkt;

// CC1101 CHIP configuration - REG -> VALUE
// to be reviewed and moved platform independed (PROGMEM)...
#include "CC1101_simple_link_reg_config.h"

/*------------  PLATFORM depended -------------------------------------------*/
void cc1101_arch_enable(void) {
  /* Set CSn to low (0) */
  CC1100_CS_PORT &= ~_BV(CC1100_CS_PIN);

  /* The MISO pin should go high before chip is fully enabled. */
  while((SPI_IN & _BV(SPI_MISO)) != 0);
}

/*---------------------------------------------------------------------------*/
void cc1101_arch_disable(void) {
  /* Set CSn to high (1) */
  CC1100_CS_PORT |= _BV(CC1100_CS_PIN);
}

/*---------------------------------------------------------------------------*/
int spi_rw_byte(unsigned char c) {
  SPDR = c;                     // send byte
  while (!(SPSR & _BV (SPIF))); // wait until transfer finished
  return SPDR;
}

#define SPI_PRESCALER_DIV128_gc (0x03<<0)
#define SPI_PRESCALER_DIV64_gc (0x02<<0)
#define SPI_PRESCALER_DIV16_gc (0x01<<0)

/*---------------------------------------------------------------------------*/
void cc1101_arch_init(void) {
  /* SPI init */
  /* Initalize ports for communication with SPI units. */
  /* CSN=SS and must be output when master! */
  SPI_DDR  |= _BV(SPI_MOSI) | _BV(SPI_SCLK);
  SPI_PORT |= _BV(SPI_MOSI) | _BV(SPI_SCLK);

  /* Unselect radio. */
  CC1100_CS_DDR |= _BV(CC1100_CS_PIN);
  cc1101_arch_disable();

  /* Enables SPI, selects "master", clock rate FCK / 2, and SPI mode 0 */
  SPCR = SPI_PRESCALER_DIV128_gc | _BV(SPE) | _BV(MSTR);
  SPSR = _BV(SPI2X);

  _delay_us(50);
}

/*---------------------------------------------------------------------------*/
void
cc1101_arch_interrupt_enable(void)
{
        /* Enable inputs on GDO pins */

        CC1100_GDO2_DDR  &= ~_BV(CC1100_GDO2_PIN);
        CC1100_GDO2_PORT |= _BV(CC1100_GDO2_PIN);

        CC1100_GDO0_DDR  &= ~_BV(CC1100_GDO0_PIN);
        CC1100_GDO0_PORT |= _BV(CC1100_GDO0_PIN);

}


//-------- basic SPI communication & helpers --------------------------

static uint8_t cc1100_readReg(uint8_t addr) {
  cc1101_arch_enable();
  spi_rw_byte( addr|CC1100_READ_BURST );
  uint8_t ret = spi_rw_byte( 0 );
  cc1101_arch_disable();
  return ret;
}

static void cc1100_writeReg(uint8_t addr, uint8_t data) {
  cc1101_arch_enable();
  spi_rw_byte( addr|CC1100_WRITE_BURST );
  spi_rw_byte( data );
  cc1101_arch_disable();
}

static uint8_t ccStrobe(uint8_t strobe) {
  cc1101_arch_enable();
  uint8_t ret = spi_rw_byte( strobe );
  cc1101_arch_disable();
  return ret;
}

void slink_done(void) {

  if (ccinitialized) {
    ccStrobe(CC1100_SIDLE);
    ccinitialized = 0;
    ccStrobe(CC1100_SRES);
  }

  SPSR = 0;
  SPCR = 0;
  SPI_PORT = 0;
  SPI_DDR  = 0;
  CC1100_CS_PORT = 0;
  CC1100_CS_DDR  = 0;
  CC1100_GDO0_PORT = 0;
  CC1100_GDO0_DDR  = 0;
  CC1100_GDO2_PORT = 0;
  CC1100_GDO2_DDR  = 0;
}

/** Turn the radio on. */
int on(void){

  // already in RX?
  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_RX)
    return 1;

  PRINTF("CC1101 DEBUG: On!\r\n");
  ccStrobe( CC1100_SIDLE );
  while((cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_IDLE));
  ccStrobe( CC1100_SFTX  );
  ccStrobe( CC1100_SFRX  );
  ccStrobe( CC1100_SRX   );
  while((cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_RX));
  return 1;
};

/** Turn the radio off. */
int off(void){
  PRINTF("CC1101 DEBUG: Off!\r\n");
  ccStrobe(CC1100_SIDLE);
  return 1;
};

//--------- interface 

/** init the radio */
void slink_init(uint16_t channel) {
  uint8_t i, reg;

  ringbuf_init( &rxbuf, RXBUF, sizeof(RXBUF));
  ringbuf_init( &txbuf, TXBUF, sizeof(TXBUF));

  cc1101_arch_init();

  ccStrobe(CC1100_SRES);
  _delay_us(200);

  ccStrobe(CC1100_SIDLE);

  // load configuration
  for (i = 0; i<200; i += 2) {
    reg = CFG(i);
    if (reg>0x40)
      break;

    cc1100_writeReg( reg, CFG(i+1) );
  }

  // assume channel is used as Sync-word
  cc1100_writeReg( 4, (channel>>8));
  cc1100_writeReg( 5, (channel&0xff));

  // just for fun, perform manual calibration (SRX would also do it automatically ...)
  ccStrobe( CC1100_SCAL );
  while(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_IDLE);

  cc1101_arch_interrupt_enable();

  PRINTF("CC1101 DEBUG: init complete!\r\n");

  txseq = 1;
  rxseq = 0;
#ifdef RADIO_CRYPT
  do_crypt = 0;
#endif

  memset( &txpkt, 0, sizeof( txpkt));
  ccinitialized = 1;
};

void slink_crypt(unsigned char *Key) {
#ifdef RADIO_CRYPT
  if (Key==NULL) {
    do_crypt = 0;
  } else {
    do_crypt = 1;
  }
  AES_Key = Key;
#endif
};

void slink_send(struct apkt *pkt) {
  uint8_t i, l;

  if (!pkt)
    return;

  if (pkt->plen>63)
    return;

#ifdef RADIO_CRYPT
  if (do_crypt && pkt->start) {
    pkt->plen = 0;
    PRINTF("CC1101 DEBUG: encrypting pkt...\r\n");
    aes128_init(AES_Key, &ctx);
    l = pkt->len + 2;
    for (i=0; i<l; i+=(AES_KEY_SIZE/8)) {
      aes128_enc(((unsigned char *)pkt)+i, &ctx);
      pkt->plen += (AES_KEY_SIZE/8);
    }
    pkt->start = 0;
  }
#endif

  // fill FIFO
  cc1101_arch_enable();

  spi_rw_byte(CC1100_WRITE_BURST | CC1100_TXFIFO);

  // length
  spi_rw_byte(pkt->plen);

  // payload
  for(i = 0; i < pkt->plen; i++) {
    spi_rw_byte(*((uint8_t *)(pkt)+pkt->start+i));
  }

  cc1101_arch_disable();

  // check CCA before "talking"
  while ((cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_RX) && !bit_is_set(CC1100_GDO0_IN, CC1100_GDO0_PIN)) {
    PRINTF("CC1101 DEBUG: CCA busy, waiting ... \r\n");
    _delay_ms(200);
  }

  ccStrobe(CC1100_STX);
  while(cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_RX);

  PRINTF("CC1101 DEBUG: send %d bytes seq: %d!\r\n", pkt->plen, pkt->seq);
}

static void slink_flush(void) {

  // is last packet not ack'd yet?
  if (txpkt.timeout) {

    // check resent here ...
    if (--txpkt.timeout==0) {
      PRINTF("CC1101 DEBUG: resending ...\r\n");
      goto resent_pkt; 
    }

    return;
  }

  // anything new to send?
  if (ringbuf_elements( &txbuf )==0)
    return;

  // create a fresh packet for TX
  memset( &txpkt, 0, sizeof( txpkt));
  txpkt.seq  = txseq++;
  txpkt.cseq = txpkt.seq;
  txpkt.len = 0;
  while (ringbuf_elements( &txbuf )) {
    txpkt.data[txpkt.len++] = ringbuf_get( &txbuf );
    if (txpkt.len>=CC1101_MAX_PAYLOAD)
      break;
  }
  // set start and lenght of payload for uncrypted packet:
  txpkt.start = 1;
  txpkt.plen = txpkt.len + 1;
  PRINTF("CC1101 DEBUG: preparing new pkt...\r\n");

resent_pkt:
  txpkt.timeout = RESEND_AFTER;
  
  // send "the" txpkt ...
  slink_send( &txpkt );

  on();
};

uint8_t slink_recv(struct apkt *pkt) {
  uint8_t i;

  on();
/*
  // check state of RF chip
  i = cc1100_readReg( CC1100_MARCSTATE ); 
  switch(i) {
    case MARCSTATE_IDLE:
    case MARCSTATE_TXFIFO_UNDERFLOW:
    case MARCSTATE_RXFIFO_OVERFLOW:
      PRINTF("CC1101 DEBUG: lost RX by: %d\r\n", i);
      on();
    default:
      break;
  }
*/

  if(!pkt)
    return SL_ERROR;

  memset( pkt, 0, sizeof( struct apkt ));

  // is something in RX fifo?
  if (!bit_is_set(CC1100_GDO2_IN, CC1100_GDO2_PIN))
    return SL_NONE;

  // read payload len from FIFO
  pkt->plen = cc1100_readReg( CC1100_RXFIFO ); // read len

  if ((pkt->plen==0) || (pkt->plen>63)) {
    ccStrobe( CC1100_SIDLE  );
    PRINTF("CC1101 DEBUG: Packet len %d is invalid!\r\n", pkt->plen);
    return SL_ERROR;
  }

  pkt->start = 1;
  pkt->len = pkt->plen-1;

#ifdef RADIO_CRYPT
  if (do_crypt)
    pkt->start = 0;
#endif

  PRINTF("CC1101 DEBUG: Reading %d bytes!\r\n", pkt->plen);

  // read all data in FIFO
  cc1101_arch_enable();
  spi_rw_byte(CC1100_READ_BURST | CC1100_RXFIFO);

  for (i=0; i<pkt->plen; i++) {
    *((uint8_t *)(pkt)+pkt->start+i) = spi_rw_byte( 0 );
  }

  // misuse some vars for storage
  pkt->cseq    = spi_rw_byte( 0 ); // RSSI
  pkt->timeout = spi_rw_byte( 0 ); // LQI

  cc1101_arch_disable();

  // eat up the rest, if any ...
  while (cc1100_readReg(CC1100_RXBYTES) & 0x7f)	
    cc1100_readReg( CC1100_RXFIFO );

  if (!( pkt->timeout & CC1100_LQI_CRC_OK_BM )) {
    ccStrobe( CC1100_SIDLE  );
    PRINTF("CC1101 DEBUG: Packet was not CRC_OK!\r\n");
    return SL_ERROR;
  }

#ifdef RADIO_CRYPT
  if (do_crypt) {
    PRINTF("CC1101 DEBUG: decrypting pkt...\r\n");
    aes128_init(AES_Key, &ctx);
    for (i=0; i<pkt->plen; i+= (AES_KEY_SIZE/8)) {
      aes128_dec(((unsigned char *)pkt)+i, &ctx);
    }
    pkt->start = 1;
  }
#endif

  PRINTF("CC1101 DEBUG: Reading %d bytes - seq: %d!\r\n", pkt->len, pkt->seq);

  return SL_OK;
}

static void slink_receive(void) {
  uint8_t i;
  struct apkt p;

  // get the packet ...
  if(slink_recv( &p ) != SL_OK)
    return;

  if (p.len==0) {
    PRINTF("CC1101 DEBUG: Rcvd Ack for %d!\r\n", p.seq);

    if (txpkt.cseq == p.seq)
      txpkt.timeout = 0;

    return;
  }
  
  // check if our ACK was lost, so a "resent" is coming in ...
  if (p.seq == rxseq) {
    PRINTF("CC1101 DEBUG: We already have sequenz %d, so ACKing it ...\r\n", p.seq);
    goto ack_pkt;
  // check sequence continuety
  } else if (p.seq!=(uint8_t)(rxseq+1)) {
    ccStrobe( CC1100_SIDLE  );
    PRINTF("CC1101 DEBUG: Sequenz %d mismatch we are at %d!\r\n", p.seq, rxseq);
    return;
 }

  // takeover data 
  for (i=0; i<p.len; i++) {
    ringbuf_put( &rxbuf, p.data[i] );
  }
  
ack_pkt:
 
  // send back ACK

  PRINTF("CC1101 DEBUG: sending ACK for %d!\r\n", p.seq);

  rxseq   = p.seq; // remember 
  p.len   = 0;
  p.plen  = 1;
  p.start = 1;

  slink_send( &p );

};

int slink_put(uint8_t c) {
  int res;
  res = ringbuf_put( &txbuf, c );
  if (ringbuf_elements( &txbuf )>=CC1101_MAX_PAYLOAD)
    slink_flush();
  return res;
};

int slink_get(void) {
  return ringbuf_get( &rxbuf );
};

int slink_avail(void) {
//  slink_flush();
  return ringbuf_size( &txbuf )-ringbuf_elements( &txbuf );
};

int slink_size(void) {
  return ringbuf_size( &rxbuf );
};

// this function should be polled
int slink_elements(void) { 
  slink_receive();
  slink_flush();

  return ringbuf_elements( &rxbuf );
};

#endif
