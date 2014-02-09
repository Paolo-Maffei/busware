/**
 * Copyright (c) 2012, Timothy Rule <trule.github@nym.hush.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @file
 * 		XMEGA TWI (I2C) Master.
 * @author
 * 		Timothy Rule <trule.github@nym.hush.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "twi-master.h"


#define COMPLETED		0
#define MASTER_BUSY		-1
#define BUS_ERROR		-2
#define UNEXP_NACK		-3
#define NO_RIF_ON_RX	-4


//#define DEBUG
#ifdef DEBUG
#define dprintf(FORMAT, args...) printf_P(PSTR(FORMAT), ##args)
#else
#define dprintf(...)
#endif


/**
 * twi_init
 */
int
twi_init(TWI_t* twi, uint8_t baud)
{
	twi->MASTER.BAUD = baud;
	return 0;
}

/**
 * twi_finish
 */
static int
twi_finish(TWI_t* twi, int rc)
{
	dprintf("TWI - finish with rc %d\n", rc);

	twi->MASTER.CTRLC = (twi->MASTER.CTRLC & ~TWI_MASTER_CMD_gm) |
			TWI_MASTER_CMD_STOP_gc;
	twi->MASTER.CTRLA &= ~TWI_MASTER_ENABLE_bm;

	return rc;
}

/**
 * twi_transaction
 *
 * @note	Once the bus is taken always finish the transaction with a call
 * 			to function twi_finish. This ensures that a STOP is put on the bus
 * 			and that the master is disabled (as per the following note).
 *
 * @note	Master is enabled on entry, the bus is forced to idle state, and
 * 			then the master is disabled on exit. This ensures that the bus never
 * 			gets stuck in Owner state. Other mechanisms exist to achieve the
 * 			same effect (timeouts, retry's etc) and are left as an exercise
 * 			for the reader.
 *
 * @note	Multi-master mode is not supported however such support could be
 * 			easily added along with changes suggested in above note.
 */
int
twi_transaction(TWI_t* twi, uint8_t address, uint8_t* write,
		uint16_t write_len, uint8_t* read, uint16_t read_len)
{
	dprintf("TWI - transaction on master 0x%04x to slave 0x%02x\n",
			(uint16_t)twi, address);

	/* Check master is idle, if not another "process" is using it. */
	if (twi->MASTER.CTRLA & TWI_MASTER_ENABLE_bm)
		return MASTER_BUSY;

	twi->MASTER.CTRLA |= TWI_MASTER_ENABLE_bm;
	twi->MASTER.STATUS = (twi->MASTER.STATUS & ~TWI_MASTER_BUSSTATE_gm) |
			TWI_MASTER_BUSSTATE_IDLE_gc;
	twi->MASTER.CTRLC &= ~TWI_MASTER_ACKACT_bm;

	/* Write part of transaction. */
	if (write_len) {
		/* Send write address. */
		twi->MASTER.ADDR = address & 0xfe;
		while (!(twi->MASTER.STATUS & TWI_MASTER_WIF_bm));
		if (twi->MASTER.STATUS & TWI_MASTER_BUSERR_bm)
			return twi_finish(twi, BUS_ERROR);
		if (twi->MASTER.STATUS & TWI_MASTER_RXACK_bm)
			return twi_finish(twi, UNEXP_NACK);

		/* Send data. */
		while (write_len-- > 0) {
			dprintf("TWI -     0x%02x\n", *write);
			twi->MASTER.DATA = *write;
			write++;
			while (!(twi->MASTER.STATUS & TWI_MASTER_WIF_bm));
			if (twi->MASTER.STATUS & TWI_MASTER_BUSERR_bm)
				return twi_finish(twi, BUS_ERROR);
			if (twi->MASTER.STATUS & TWI_MASTER_RXACK_bm)
				return twi_finish(twi, (read_len || write_len) ?
						UNEXP_NACK : COMPLETED);
		}
	}

	/* Read part of transaction. */
	if (read_len) {
		/* Send read address. */
		twi->MASTER.ADDR = address | 0x01;

		do {
			/* Read data. */
			while (!(twi->MASTER.STATUS &
					(TWI_MASTER_RIF_bm | TWI_MASTER_WIF_bm)));
			if (twi->MASTER.STATUS & TWI_MASTER_BUSERR_bm)
				return twi_finish(twi, BUS_ERROR);
			if (twi->MASTER.STATUS & TWI_MASTER_RXACK_bm)
				return twi_finish(twi, UNEXP_NACK);
			if (!(twi->MASTER.STATUS & TWI_MASTER_RIF_bm))
				return twi_finish(twi, NO_RIF_ON_RX);

			*read++ = twi->MASTER.DATA;
			dprintf("TWI -           0x%02x\n", *(read - 1));

			if (--read_len)
				/* Read the next byte. */
				twi->MASTER.CTRLC = (twi->MASTER.CTRLC & ~TWI_MASTER_CMD_gm) |
						TWI_MASTER_CMD_RECVTRANS_gc;
		} while (read_len);
	}

	/* Finish with a NACK. */
	twi->MASTER.CTRLC |= TWI_MASTER_ACKACT_bm;
	return twi_finish(twi, COMPLETED);
}
