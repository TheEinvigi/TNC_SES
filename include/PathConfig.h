/*
 * path.h
 *
 *  Created on: 02.08.2017
 *      Author: mateusz
 */

#ifndef PATH_H_
#define PATH_H_

#include <stored_configuration_nvm/config_data.h>
#include "aprs/ax25.h"

  /* C++ detection */
  #ifdef __cplusplus
  extern "C" {
  #endif

  uint8_t ConfigPath(AX25Call* p, const config_data_basic_t* conf);

  #ifdef __cplusplus
  }
  #endif


#endif /* PATH_H_ */
