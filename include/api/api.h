/*
 * api.h
 *
 *  Created on: Apr 24, 2022
 *      Author: mateusz
 */

#ifndef API_H_
#define API_H_

void api_init(char * api_base, char * station_name);
void api_calculate_mac(void);
void api_send_json_status(void);
void api_send_json_measuremenets(void);

#endif /* API_H_ */
