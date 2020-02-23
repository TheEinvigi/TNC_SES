/*
 * umb_state_t.h
 *
 *  Created on: 23.02.2020
 *      Author: mateusz
 */

#ifndef INCLUDE_UMB_MASTER_UMB_STATE_T_H_
#define INCLUDE_UMB_MASTER_UMB_STATE_T_H_

typedef enum umb_state_t {
	UMB_STATUS_IDLE,
	UMB_STATUS_SENDING_REQUEST_TO_SLAVE,
	UMB_STATUS_WAITING_FOR_RESPONSE,
	UMB_STATUS_RESPONSE_AVALIABLE,
	UMB_STATUS_ERROR
} umb_state_t;

#endif /* INCLUDE_UMB_MASTER_UMB_STATE_T_H_ */
