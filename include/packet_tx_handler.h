/*
 * packet_tx_handler.h
 *
 *  Created on: 26.01.2019
 *      Author: mateusz
 */

#ifndef PACKET_TX_HANDLER_H_
#define PACKET_TX_HANDLER_H_

#include "config_data.h"

typedef struct packet_tx_counter_values_t {

	uint8_t beacon_counter;
	uint8_t wx_counter;
	uint8_t telemetry_counter;
	uint8_t telemetry_desc_counter;
	uint8_t kiss_counter;

} packet_tx_counter_values_t;

void packet_tx_send_wx_frame(void);
void packet_tx_configure(uint8_t meteo_interval, uint8_t beacon_interval, config_data_powersave_mode_t powersave);
void packet_tx_tcp_handler(void);
void packet_tx_handler(const config_data_basic_t * const config_basic, const config_data_mode_t * const config_mode);
void packet_tx_get_current_counters(packet_tx_counter_values_t * out);
void packet_tx_set_current_counters(packet_tx_counter_values_t * in);
int16_t packet_tx_get_minutes_to_next_wx(void);

#endif /* PACKET_TX_HANDLER_H_ */
