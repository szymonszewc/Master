/*
 * laptime.c
 *
 *  Created on: May 14, 2021
 *      Author: Alicja Miekina
 */

/* stan przycisku speedReset resetuje czas okrazenia i srednia przedkosc
 * gdy predkosc pojazdu wyniesie wiecej niz 0 oraz zostanie wcisniety
 * przycisk powerSupply zostanie uruchomione zliczanie czasu
 * zmiana stanu na speedReset zatrzymuje czas i resetuje srednia i predkosc
 * czas okrazenia przeliczany jest na minuty, sekundy, milisekundy i wysylany do kierownicy


#include "rs485.h"
#include "hydrogreen.h"
#include "laptime.h"

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

LAPTIME_FROM_SENSOR LAPTIME;
void laptime_step(void)
{
	if(HAL_ADC_GetValue(&hadc2) > 3)
	{
		HAL_TIM_Base_Start_IT(&htim6);
		if(RS485_RX_VERIFIED_DATA_SW.speedReset == 1)
		{
			HAL_TIM_Base_Stop_IT(&htim6);
			laptime_miliseconds= HAL_TIM_Base_GetState(&htim6);
			laptime_seconds = laptime_miliseconds/1000;
			laptime_minutes = laptime_seconds/60;
			HAL_TIM_STATE_RESET;
			LAPTIME.laptime_miliseconds = laptime_miliseconds;
			LAPTIME.laptime_seconds = laptime_seconds;
			LAPTIME.laptime_minutes = laptime_minutes;
		}
	}
}
*/
