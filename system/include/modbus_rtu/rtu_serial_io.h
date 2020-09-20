/*
 * rtu_serial_io.h
 *
 *  Created on: 27.08.2020
 *      Author: mateusz
 */

#ifndef INCLUDE_MODBUS_RTU_RTU_SERIAL_IO_H_
#define INCLUDE_MODBUS_RTU_RTU_SERIAL_IO_H_

#include <stdint.h>

#include "./drivers/serial.h"

#include "./modbus_rtu/rtu_pool_queue_t.h"

extern uint16_t rtu_serial_previous_crc;

inline void rtu_serial_reset_crc(void) {
	rtu_serial_previous_crc = 0xFFFF;
}

uint8_t rtu_serial_callback(uint8_t current_data, const uint8_t * const rx_buffer, uint16_t rx_bytes_counter);

int32_t rtu_serial_init(rtu_pool_queue_t* queue);
int32_t rtu_serial_pool(rtu_pool_queue_t* queue, srl_context_t* serial_context);
int32_t rtu_serial_start(void);


#endif /* INCLUDE_MODBUS_RTU_RTU_SERIAL_IO_H_ */