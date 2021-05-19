/*
 * laptime.h
 *
 *  Created on: May 14, 2021
 *      Author: Alicja Miekina
 */

#ifndef LAPTIME_H_
#define LAPTIME_H_


#pragma once

#include <stdint-gcc.h>

#endif /* LAPTIME_H_ */

typedef struct
			{
				 union
				  {
				    uint16_t value;
				    char array[2];
				  } laptime_minutes;

				  union
				  {
				    uint16_t value;
				    char array[2];
				  } laptime_miliseconds;

				  uint8_t laptime_seconds;
					} LAPTIME_FROM_SENSOR;
				extern LAPTIME_FROM_SENSOR LAPTIME;

extern void laptime_step(void);
