/**
* @file rs485.h
* @brief Biblioteka do obslugi komunikacji UART <-> RS485 <-> UART
* @author Piotr Durakiewicz
* @date 08.12.2020
* @todo
* @bug
* @copyright 2020 HYDROGREEN TEAM
*/
#pragma once

#include <stdint-gcc.h>

// ******************************************************************************************************************************************************** //

#define RS485_FLT_NONE_SW 0x00						///< Brak bledu
#define RS485_NEW_DATA_TIMEOUT_SW 0x11				///< Nie otrzymano nowych dane (polaczenie zostalo zerwane)

#define RS485_FLT_NONE_EF 0x00						///< Brak bledu
#define RS485_NEW_DATA_TIMEOUT_EF 0x11				///< Nie otrzymano nowych dane (polaczenie zostalo zerwane)

extern uint8_t rs485_flt_SW; 						///< Zmienna przechowujaca aktualny kod bledu magistrali
extern uint8_t rs485_flt_EF; 						///< Zmienna przechowujaca aktualny kod bledu magistrali

// ******************************************************************************************************************************************************** //

extern void rs485_init_SW(void);					///< Inicjalizacja magistrali RS-485, umiescic wewnatrz hydrogreen_init(void)
extern void rs485_step_SW(void);					///< Funkcja obslugujaca magistrale, umiescic wewnatrz hydrogreen_step(void)

extern void rs485_init_EF(void);					///< Inicjalizacja magistrali RS-485, umiescic wewnatrz hydrogreen_init(void)
extern void rs485_step_EF(void);					///< Funkcja obslugujaca magistrale, umiescic wewnatrz hydrogreen_step(void)

// ******************************************************************************************************************************************************** //

/**
* @struct RS485_RECEIVED_VERIFIED_DATA_SW
* @brief Struktura zawierajaca sprawdzone otrzymane dane od kierownicy
*/
typedef struct
{
  ///< ELEMENTY W STRUKTURZE MUSZA BYC POSORTOWANE W PORZADKU MALEJACYM
  ///< https://www.geeksforgeeks.org/is-sizeof-for-a-struct-equal-to-the-sum-of-sizeof-of-each-member/

  uint8_t halfGas;
  uint8_t fullGas;
  uint8_t horn;

  uint8_t speedReset;
  uint8_t powerSupply;

  uint8_t scClose;
  uint8_t fuelcellOff;

  uint8_t fuelcellPrepareToRace;
  uint8_t fuelcellRace;
} RS485_RECEIVED_VERIFIED_DATA_SW;
extern RS485_RECEIVED_VERIFIED_DATA_SW RS485_RX_VERIFIED_DATA_SW;
/**
* @struct RS485_NEW_DATA_SW
* @brief Struktura zawierajaca dane przesylane dla kierowcy
*/
typedef struct
{
  ///< ELEMENTY W STRUKTURZE MUSZA BYC POSORTOWANE W PORZADKU MALEJACYM
  ///< https://www.geeksforgeeks.org/is-sizeof-for-a-struct-equal-to-the-sum-of-sizeof-of-each-member/
  union
  {
    float value;
    char array[4];
  } FC_V;

  union
  {
    float value;
    char array[4];
  } FC_TEMP;

  union
  {
    float value;
    char array[4];
  } CURRENT_SENSOR_FC_TO_SC;

  union
  {
    float value;
    char array[4];
  } CURRENT_SENSOR_SC_TO_MOTOR;

  union
  {
    float value;
    char array[4];
  } SC_V;
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

  union
  {
    uint16_t value;
    char array[2];
  } fcFanRPM;

  uint8_t interimSpeed;
  uint8_t averageSpeed;
  uint8_t laptime_seconds;

  uint8_t fcToScMosfetPWM;
  uint8_t motorPWM;

  uint8_t h2SensorDigitalPin;
  uint8_t emergencyButton;
} RS485_NEW_DATA_SW;
extern RS485_NEW_DATA_SW RS485_TX_DATA_SW;
/**
* @struct RS485_RECEIVED_VERIFIED_DATA_EF
* @brief Struktura zawierajaca sprawdzone otrzymane dane od przeplywu energii
*/
typedef struct
{
  ///< ELEMENTY W STRUKTURZE MUSZA BYC POSORTOWANE W PORZADKU MALEJACYM
  ///< https://www.geeksforgeeks.org/is-sizeof-for-a-struct-equal-to-the-sum-of-sizeof-of-each-member/
  union
  {
	 float value;
	 char array[4];
  } FC_V;
  union
   {
     float value;
     char array[4];
   } FC_TEMP;
   union
    {
      float value;
      char array[4];
    } CURRENT_SENSOR_FC_TO_SC;
    union
     {
       float value;
       char array[4];
     } SC_V;
     union
     {
       uint16_t value;
       char array[2];
     } fcFanRPM;

 // uint8_t fcToScMosfetPWM;
 // uint8_t motorPWM;
  uint8_t emergency;

} RS485_RECEIVED_VERIFIED_DATA_EF;
extern RS485_RECEIVED_VERIFIED_DATA_EF RS485_RX_VERIFIED_DATA_EF;
/**
* @struct RS485_NEW_DATA_EF
* @brief Struktura zawierajaca przesylane dane do przeplywu energii
*/
typedef struct
{
	uint8_t fuellCellModeButtons;				//Stan 3 przycisków: fuellCellPrepareToRace, fuelCellRace, fuelCellRace
	uint8_t scOn;
	uint8_t emergencyScenario;
	uint8_t motorPWM;
}RS485_NEW_DATA_EF;
extern RS485_NEW_DATA_EF RS485_TX_DATA_EF;
