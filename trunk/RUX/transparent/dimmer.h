#ifndef _DIMMER_H_
#define _DIMMER_H_

uint8_t ch_level[6];
uint8_t ch_state[6];

#define CH_OFF_bm	1

void dimmer_init(void);
void dimmer_set(uint8_t channel, uint32_t value);
void dimmer_task(void);

#endif
