/*
 * max31865.h
 *
 *  Created on: Sep 25, 2022
 *      Author: mateusz
 */

#ifndef INCLUDE_DRIVERS_MAX31865_H_
#define INCLUDE_DRIVERS_MAX31865_H_

#include "config_data.h"
#include "drivers/spi.h"
#include "int_average.h"

#include <stdint.h>

#define MAX_3WIRE	3
#define MAX_4WIRE	1

typedef enum max31865_qf_t {
	MAX_QF_UNKNOWN,
	MAX_QF_FULL,
	MAX_QF_NOT_AVALIABLE
}max31865_qf_t;

extern uint8_t max31865_current_fault_status;
extern uint8_t max31865_measurements_counter;
extern int_average_t max31865_average;

void max31865_init(uint8_t rdt_type, uint8_t reference_resistor);
void max31865_init_average(void);
void max31865_pool(void);
int32_t max31865_get_pt100_result();
int32_t max31865_get_result(uint32_t RTDnominal);
max31865_qf_t max31865_get_qf(void);

#endif /* INCLUDE_DRIVERS_MAX31865_H_ */