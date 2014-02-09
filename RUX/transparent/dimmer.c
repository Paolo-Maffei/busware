#include <avr/io.h>
#include <string.h>
#include "dimmer.h"
#include "twi-master.h"

#define PCA9532_ADDR 0xc0 
/* PCA9532 is connected to TWI Master on Port E. */
#define I2C(a,b,c,d)    twi_transaction(&TWIE, PCA9532_ADDR, (a), (b), (c), (d));

#define MAXPER 	25500

#define CH1_LD	TCF0.CCA
#define CH1_PWM	TCF0.CCB

#define CH2_LD	TCD1.CCB
#define CH2_PWM	TCD1.CCA

#define CH3_LD	TCD0.CCD
#define CH3_PWM	TCD0.CCC

#define CH4_LD	TCD0.CCB
#define CH4_PWM	TCD0.CCA

#define CH5_LD	TCC0.CCC
#define CH5_PWM	TCC0.CCD

#define CH6_LD	TCC0.CCA
#define CH6_PWM	TCC0.CCB

static uint32_t levels[] = {
    0x00602000, 0x00802000, 0x00a02000, 0x00c02000, 0x01002000, 0x02002000, 0x08002000, 0x10002000, 0x20002000, 0x40002000, 0x80002000, 0x2000, 0x3000, 0x4000, 0x5000, MAXPER
};

void dimmer_init(void) {

        PORTF.DIR |= PIN0_bm | PIN1_bm;
        PORTD.DIR |= PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
        PORTC.DIR |= PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;

        TCF0.CTRLA = TC_CLKSEL_DIV1_gc;
        TCF0.CTRLB = (TC0_CCAEN_bm|TC0_CCBEN_bm|TC_WGMODE_SS_gc);
        TCF0.PER   = MAXPER;

        TCD0.CTRLA = TC_CLKSEL_DIV1_gc;
        TCD0.CTRLB = (TC0_CCAEN_bm|TC0_CCBEN_bm|TC0_CCCEN_bm|TC0_CCDEN_bm|TC_WGMODE_SS_gc);
        TCD0.PER   = MAXPER;

        TCD1.CTRLA = TC_CLKSEL_DIV1_gc;
        TCD1.CTRLB = (TC1_CCAEN_bm|TC1_CCBEN_bm|TC_WGMODE_SS_gc);
        TCD1.PER   = MAXPER;

        TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
        TCC0.CTRLB = (TC0_CCAEN_bm|TC0_CCBEN_bm|TC0_CCCEN_bm|TC0_CCDEN_bm|TC_WGMODE_SS_gc);
        TCC0.PER   = MAXPER;

	memset( ch_level, 11, sizeof( ch_level));
	memset( ch_state, CH_OFF_bm, sizeof( ch_state));
}

// value - high 16 bits are PWM // lower 16 bits are LD
void dimmer_set(uint8_t channel, uint32_t value) {
	switch (channel) {
		case 1:
			CH1_LD  = (value&0xffff);
			if (CH1_LD) TCF0.CTRLB  |= TC0_CCAEN_bm; else { TCF0.CTRLB &= ~TC0_CCAEN_bm; PORTF.OUT &= ~PIN0_bm; }
			CH1_PWM = (value>>16);
			if (CH1_PWM) TCF0.CTRLB |= TC0_CCBEN_bm; else { TCF0.CTRLB &= ~TC0_CCBEN_bm; PORTF.OUT |= PIN1_bm; }
			break;
		case 2:
			CH2_LD  = (value&0xffff);
			if (CH2_LD) TCD1.CTRLB  |= TC1_CCBEN_bm; else { TCD1.CTRLB &= ~TC1_CCBEN_bm; PORTD.OUT &= ~PIN5_bm; }
			CH2_PWM = (value>>16);
			if (CH2_PWM) TCD1.CTRLB |= TC1_CCAEN_bm; else { TCD1.CTRLB &= ~TC1_CCAEN_bm; PORTD.OUT |= PIN4_bm; }
			break;
		case 3:
			CH3_LD  = (value&0xffff);
			if (CH3_LD) TCD0.CTRLB  |= TC0_CCDEN_bm; else { TCD0.CTRLB &= ~TC0_CCDEN_bm; PORTD.OUT &= ~PIN3_bm; }
			CH3_PWM = (value>>16);
			if (CH3_PWM) TCD0.CTRLB |= TC0_CCCEN_bm; else { TCD0.CTRLB &= ~TC0_CCCEN_bm; PORTD.OUT |= PIN2_bm; }
			break;
		case 4:
			CH4_LD  = (value&0xffff);
			if (CH4_LD) TCD0.CTRLB  |= TC0_CCBEN_bm; else { TCD0.CTRLB &= ~TC0_CCBEN_bm; PORTD.OUT &= ~PIN1_bm; }
			CH4_PWM = (value>>16);
			if (CH4_PWM) TCD0.CTRLB |= TC0_CCAEN_bm; else { TCD0.CTRLB &= ~TC0_CCAEN_bm; PORTD.OUT |= PIN0_bm; }
			break;
		case 5:
			CH5_LD  = (value&0xffff);
			if (CH5_LD) TCC0.CTRLB  |= TC0_CCCEN_bm; else { TCC0.CTRLB &= ~TC0_CCCEN_bm; PORTC.OUT &= ~PIN2_bm; }
			CH5_PWM = (value>>16);
			if (CH5_PWM) TCC0.CTRLB |= TC0_CCDEN_bm; else { TCC0.CTRLB &= ~TC0_CCDEN_bm; PORTC.OUT |= PIN3_bm; }
			break;
		case 6:
			CH6_LD  = (value&0xffff);
			if (CH6_LD) TCC0.CTRLB  |= TC0_CCAEN_bm; else { TCC0.CTRLB &= ~TC0_CCAEN_bm; PORTC.OUT &= ~PIN0_bm; }
			CH6_PWM = (value>>16);
			if (CH6_PWM) TCC0.CTRLB |= TC0_CCBEN_bm; else { TCC0.CTRLB &= ~TC0_CCBEN_bm; PORTC.OUT |= PIN1_bm; }
			break;
	}
}

void dimmer_task(void) {
 	uint8_t led_bm = 0;
        uint8_t i2c_cmd[5];

	// updates LEDs
	i2c_cmd[0] = 0x16;
	i2c_cmd[1] = 0;
	i2c_cmd[2] = 0;

	for (uint8_t i=0; i<6; i++) {
		ch_level[i] &= 0xf;
		if (ch_state[i] & CH_OFF_bm) { 
			dimmer_set( i+1, 0x00010000 );
			led_bm = 0;
		} else { 
			dimmer_set( i+1, levels[ch_level[i]] );
			switch (ch_level[i]) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					led_bm = 2;
					break;
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
					led_bm = 3;
					break;
				default:
					led_bm = 1;
			}
			switch (i) {
				case 0:
                                	i2c_cmd[2] |= (led_bm<<2);
					break;
				case 1:
                                	i2c_cmd[2] |= (led_bm<<0);
					break;
				case 2:
                                	i2c_cmd[1] |= (led_bm<<6);
					break;
				case 3:
                                	i2c_cmd[1] |= (led_bm<<4);
					break;
				case 4:
                                	i2c_cmd[1] |= (led_bm<<2);
					break;
				case 5:
                                	i2c_cmd[1] |= (led_bm<<0);
					break;
			}
		}


	}
        I2C( i2c_cmd, 3, NULL, 0);
}
