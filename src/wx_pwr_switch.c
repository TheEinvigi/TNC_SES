/*
 * pwr_switch.c
 *
 *  Created on: Aug 31, 2021
 *      Author: mateusz
 */

#include <wx_pwr_switch.h>
#include "station_config.h"
#include "main.h"
#include "rte_wx.h"
#include "wx_handler.h"
#include "io.h"
#include "pwr_save.h"
#include "delay.h"


#ifdef STM32F10X_MD_VL
#include <stm32f10x.h>
#endif

#ifdef STM32L471xx
#include <stm32l4xx.h>
#include <stm32l4xx_ll_gpio.h>
#endif

/**
 * This power state applies only to ParaTNC or ParaMETEO with powersaving disabled. Otherwise
 * swithig on / off is handled by pwr_save.c
 */
wx_pwr_state_t wx_pwr_state;

#define REGISTER RTC->BKP0R

#define WX_WATCHDOG_PERIOD (SYSTICK_TICKS_PER_SECONDS * SYSTICK_TICKS_PERIOD * 90)
#define WX_WATCHDOG_RESET_DURATION (SYSTICK_TICKS_PER_SECONDS * SYSTICK_TICKS_PERIOD * 3)

void wx_pwr_switch_case_under_reset_parameteo() {

	if (pwr_save_get_inhibit_pwr_switch_periodic() == 1)
		return;

	io___cntrl_vbat_s_enable();

	wx_force_i2c_sensor_reset = 1;

	wx_pwr_state = WX_PWR_ON;

}

void wx_pwr_switch_case_under_reset_paratnc() {

	// Turn on the +5V_ISOL (VDD_SW) voltage
	io_5v_isol_sw_enable();

#ifdef PWR_SWITCH_BOTH
	io_12v_sw_enable();

	wx_force_i2c_sensor_reset = 1;
#endif

	wx_pwr_state = WX_PWR_ON;
}

void wx_pwr_switch_case_off_parameteo() {

	if (pwr_save_get_inhibit_pwr_switch_periodic() == 1)
		return;

	// Turn on the +5V_ISOL (VDD_SW) voltage
	io___cntrl_vbat_s_enable();

	wx_force_i2c_sensor_reset = 1;

	wx_pwr_state = WX_PWR_ON;
}

void wx_pwr_switch_case_off_paratnc() {
#if (defined STM32F10X_MD_VL)
		// Turn on the +12V_SW voltage
		GPIO_SetBits(GPIOA, GPIO_Pin_6);
#endif

		delay_fixed(100);

		// Turn on the +5V_ISOL (VDD_SW) voltage
		//GPIO_SetBits(GPIOB, GPIO_Pin_8);
		io_5v_isol_sw_enable();
}

void wx_pwr_switch_init(void) {

}

void wx_pwr_switch_periodic_handle(void) {

	// do a last valid measuremenets timestamps only if power is currently applied
	if (wx_pwr_state == WX_PWR_ON) {

		// the value of 0xFFFFFFFF is a magic word which disables the check for this parameter
		if (wx_last_good_temperature_time != 0xFFFFFFFF &&
			master_time - wx_last_good_temperature_time >= WX_WATCHDOG_PERIOD)
		{
			wx_pwr_state = WX_PWR_UNDER_RESET;
		}

		// as the weather station could be configured not to perform wind measurements at all
		if (wx_last_good_wind_time != 0xFFFFFFFF &&
			master_time - wx_last_good_wind_time >= WX_WATCHDOG_PERIOD)
		{
			wx_pwr_state = WX_PWR_UNDER_RESET;

			rte_wx_wind_qf = AN_WIND_QF_DEGRADED;
		}

		if (wx_pwr_state == WX_PWR_UNDER_RESET) {
			// if timeout watchod expired there is a time to reset the supply voltage
			wx_pwr_state = WX_PWR_UNDER_RESET;

#if (defined STM32F10X_MD_VL)
			// pull the output down to switch the relay and disable +5V_ISOL (VDD_SW)
			io_5v_isol_sw_disable();

#ifdef PWR_SWITCH_BOTH
			io_12v_sw_disable();
#endif
#endif

#if (defined STM32L471xx)
			io___cntrl_vbat_s_disable();
			io___cntrl_vbat_m_disable();
#endif
			// setting the last_good timers to current value to prevent reset loop
			wx_last_good_temperature_time = master_time;
			wx_last_good_wind_time = master_time;

			return;
		}

	}

	// service actual supply state
	switch (wx_pwr_state) {
	case WX_PWR_OFF:

		// one second delay
		delay_fixed(2000);

		#if (defined STM32F10X_MD_VL)
		wx_pwr_switch_case_off_paratnc();
		#endif

		#if (defined STM32L471xx)
		wx_pwr_switch_case_off_parameteo();
		#endif

		// power is off after power-up and needs to be powered on
		wx_pwr_state = WX_PWR_ON;
		break;
	case WX_PWR_ON:
		break;
	case WX_PWR_UNDER_RESET:

		#if (defined STM32F10X_MD_VL)
		wx_pwr_switch_case_under_reset_paratnc();
		#endif

		#if (defined STM32L471xx)
		wx_pwr_switch_case_under_reset_parameteo();
		#endif

		break;
	case WX_PWR_DISABLED:
		break;
	}
}
