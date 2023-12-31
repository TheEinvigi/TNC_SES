/*
 * delay.h
 *
 *  Created on: 26.01.2019
 *      Author: mateusz
 */

#ifndef DELAY_H_
#define DELAY_H_

#include "stdint.h"
#include "main.h"

extern volatile int32_t delay_cnt;

void delay_set(uint16_t delay_in_msecs, uint8_t randomize);
void delay_fixed(int32_t delay_in_msecs);
uint32_t delay_fixed_with_count(int32_t delay_in_msecs);
void delay_from_preset(void);
void delay_random(void);

void delay_stop();
void delay_start();

inline void delay_decrement_counter(void) {
	if (delay_cnt > 0)
		delay_cnt -= SYSTICK_TICKS_PERIOD;
}


#endif /* DELAY_H_ */
