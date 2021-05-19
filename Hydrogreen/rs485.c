/**
* @file rs485.c
* @brief Biblioteka do obslugi komunikacji UART <-> RS485 <-> UART
* @author Alicja Miekina na podstawie Piotra Durakiewicza
* @date 22.04.2021
* @todo
* @bug
* @copyright 2021 HYDROGREEN TEAM
*/

#include "rs485.h"
#include "usart.h"
#include "crc.h"

// ******************************************************************************************************************************************************** //
//Ramka danych z kierownica
#define UART_PORT_RS485_SW 		huart3
#define TX_FRAME_LENGHT_SW 		35					///< Dlugosc wysylanej ramki danych (z suma CRC)
#define RX_FRAME_LENGHT_SW 		11					///< Dlugosc otrzymywanej ramki danych (z suma CRC)
#define EOT_BYTE_SW			    0x17			    ///< Bajt wskazujacy na koniec ramki

// ******************************************************************************************************************************************************** //
//Ramka danych z przeplywem energii
#define UART_PORT_RS485_EF 		huart1
#define TX_FRAME_LENGHT_EF 		6					///< Dlugosc wysylanej ramki danych (z suma CRC)
#define RX_FRAME_LENGHT_EF 		21					///< Dlugosc otrzymywanej ramki danych (z suma CRC)
#define EOT_BYTE_EF			    0x17				///< Bajt wskazujacy na koniec ramki

// ******************************************************************************************************************************************************** //
//Zmienne dla transmisji danych z kierownica
volatile static uint8_t dataFromRx_SW[RX_FRAME_LENGHT_SW]; 				///< Tablica w ktorej zawarte sa nieprzetworzone przychodzace dane
volatile static uint16_t posInRxTab_SW;									///< Aktualna pozycja w tabeli wykorzystywanej do odbioru danych
volatile static uint8_t intRxCplt_SW; 									///< Flaga informujaca o otrzymaniu nowego bajtu (gdy 1 - otrzymanowy nowy bajt)
static uint8_t dataToTx_SW[TX_FRAME_LENGHT_SW]; 						///< Tablica w ktorej zawarta jest ramka danych do wyslania
static uint16_t posInTxTab_SW;											///< Aktualna pozycja w tabeli wykorzystywanej do wysylania danych
uint8_t rs485_flt_SW = RS485_NEW_DATA_TIMEOUT_SW;						///< Zmienna przechowujaca aktualny kod bledu magistrali

// ******************************************************************************************************************************************************** //
//Zmienne dla transmisji danych z przeplywem energii
volatile static uint8_t dataFromRx_EF[RX_FRAME_LENGHT_EF]; 				///< Tablica w ktorej zawarte sa nieprzetworzone przychodzace dane
volatile static uint16_t posInRxTab_EF;									///< Aktualna pozycja w tabeli wykorzystywanej do odbioru danych
volatile static uint8_t intRxCplt_EF; 									///< Flaga informujaca o otrzymaniu nowego bajtu (gdy 1 - otrzymanowy nowy bajt)
static uint8_t dataToTx_EF[TX_FRAME_LENGHT_EF]; 						///< Tablica w ktorej zawarta jest ramka danych do wyslania
static uint16_t posInTxTab_EF;											///< Aktualna pozycja w tabeli wykorzystywanej do wysylania danych
uint8_t rs485_flt_EF = RS485_NEW_DATA_TIMEOUT_EF;						///< Zmienna przechowujaca aktualny kod bledu magistrali

// ******************************************************************************************************************************************************** //

/**
* @struct RS485_BUFFER
* @brief Struktura zawierajaca bufory wykorzystywane do transmisji danych dla kierownicy
*/

typedef struct
{
  uint8_t tx;
  uint8_t rx;
} RS485_BUFFER_SW;
static RS485_BUFFER_SW RS485_BUFF_SW;

typedef struct
{
  uint8_t tx;
  uint8_t rx;
} RS485_BUFFER_EF;
static RS485_BUFFER_EF RS485_BUFF_EF;

RS485_RECEIVED_VERIFIED_DATA_SW RS485_RX_VERIFIED_DATA_SW; 				// Struktura w ktorej zawarte sa SPRAWDZONE przychodzace dane od kierownicy
RS485_NEW_DATA_SW	RS485_TX_DATA_SW;									//Struktura wysylanych danych dla kierownicy
RS485_RECEIVED_VERIFIED_DATA_EF RS485_RX_VERIFIED_DATA_EF;              //Struktura w ktorej zawarte sa SPRAWDZONE przychodzace dane od przeplywu energii
RS485_NEW_DATA_EF RS485_TX_DATA_EF;										//Struktura wysylanych danych dla przeplywu energii
// ******************************************************************************************************************************************************** //
//dla kierownicy
static void sendData_SW(void);
static void receiveData_SW(void);
static void prepareNewDataToSend_SW(void);
static void processReceivedData_SW(void);
static void resetActData_SW(void);

// ******************************************************************************************************************************************************** //
//dla przeplywu energii
static void sendData_EF(void);
static void receiveData_EF(void);
static void prepareNewDataToSend_EF(void);
static void processReceivedData_EF(void);
static void resetActData_EF(void);

// ******************************************************************************************************************************************************** //

/**
* @fn rs485_init(void)
* @brief Inicjalizacja magistrali RS-485, umiescic wewnatrz hydrogreen_init(void) dla kierownicy
*/
void rs485_init_SW(void)
{
  HAL_UART_Receive_DMA(&UART_PORT_RS485_SW, &RS485_BUFF_SW.rx, 3);				//Rozpocznij nasluchiwanie
  prepareNewDataToSend_SW();												//Przygotuj nowy pakiet danych
}
void rs485_init_EF(void)
{
  HAL_UART_Receive_DMA(&UART_PORT_RS485_EF, &RS485_BUFF_EF.rx, 1);				//Rozpocznij nasluchiwanie
  prepareNewDataToSend_EF();												//Przygotuj nowy pakiet danych
}
/**
* @fn rs485_step(void)
* @brief Funkcje obslugujace magistrale, umiescic wewnatrz hydrogreen_step(void) dla kierownicy i przeplywu energii
*/
void rs485_step_SW(void)
{
  receiveData_SW();
  sendData_SW();
}
void rs485_step_EF(void)
{
  receiveData_EF();
  sendData_EF();
}
/**
* @fn sendData(void)
* @brief Funkcja ktorej zadaniem jest obsluga linii TX, powinna zostac umieszczona w wewnatrz rs485_step() dla kierownicy
*/
static void sendData_SW(void)
{
  static uint16_t cntEndOfTxTick_SW;							//Zmienna wykorzystywana do odliczenia czasu wskazujacego na koniec transmisji

  //Sprawdz czy wyslano cala ramke danych
  if (posInTxTab_SW < TX_FRAME_LENGHT_SW)
    {
      //Nie, wysylaj dalej
      RS485_BUFF_SW.tx = dataToTx_SW[posInTxTab_SW];

      //Na czas wysylania danych wylacz przerwania
      __disable_irq();
      HAL_UART_Transmit(&UART_PORT_RS485_SW, &RS485_BUFF_SW.tx, 3, HAL_MAX_DELAY);
      __enable_irq();

      posInTxTab_SW++;
    }
  else if (cntEndOfTxTick_SW < TX_FRAME_LENGHT_SW)
    {
      //Cala ramka danych zostala wyslana, zacznij odliczac "czas przerwy" pomiedzy przeslaniem kolejnej ramki
      cntEndOfTxTick_SW++;
    }
  else
    {
      //Przygotuj nowe dane do wysylki
      cntEndOfTxTick_SW = 0;
      posInTxTab_SW = 0;

      prepareNewDataToSend_SW();
    }
}
/**
* @fn sendData(void)
* @brief Funkcja ktorej zadaniem jest obsluga linii TX, powinna zostac umieszczona w wewnatrz rs485_step() dla przeplywu energii
*/
static void sendData_EF(void)
{
  static uint16_t cntEndOfTxTick_EF;							//Zmienna wykorzystywana do odliczenia czasu wskazujacego na koniec transmisji

  //Sprawdz czy wyslano cala ramke danych
  if (posInTxTab_EF < TX_FRAME_LENGHT_EF)
    {
      //Nie, wysylaj dalej
      RS485_BUFF_EF.tx = dataToTx_EF[posInTxTab_EF];

      //Na czas wysylania danych wylacz przerwania
      __disable_irq();
      HAL_UART_Transmit(&UART_PORT_RS485_SW, &RS485_BUFF_EF.tx, 1, HAL_MAX_DELAY);
      __enable_irq();

      posInTxTab_EF++;
    }
  else if (cntEndOfTxTick_EF < TX_FRAME_LENGHT_EF)
    {
      //Cala ramka danych zostala wyslana, zacznij odliczac "czas przerwy" pomiedzy przeslaniem kolejnej ramki
      cntEndOfTxTick_EF++;
    }
  else
    {
      //Przygotuj nowe dane do wysylki
      cntEndOfTxTick_EF = 0;
      posInTxTab_EF = 0;

      prepareNewDataToSend_EF();
    }
}
/**
* @fn receiveData(void)
* @brief Funkcja ktorej zadaniem jest obsluga linii RX, umiescic wewnatrz rs485_step() dla kierownicy
*/
static void receiveData_SW(void)
{
  static uint32_t rejectedFramesInRow_SW;							//Zmienna przechowujaca liczbe straconych ramek z rzedu
  static uint32_t cntEndOfRxTick_SW;								//Zmienna wykorzystywana do odliczenia czasu wskazujacego na koniec transmisji

  //Sprawdz czy otrzymano nowe dane
  if (!intRxCplt_SW)
    {
      //Nie otrzymano nowych danych, zacznij odliczac czas
      cntEndOfRxTick_SW++;
    }
  else if (intRxCplt_SW)
    {
      //Nowe dane zostaly otrzymane, zeruj flage informujaca o zakonczeniu transmisji
      intRxCplt_SW = 0;
    }

  //Sprawdz czy minal juz czas wynoszacy RX_FRAME_LENGHT
  if (cntEndOfRxTick_SW > RX_FRAME_LENGHT_SW)
    {
      //Na czas przetwarzania danych wylacz przerwania
      __disable_irq();

      //Czas minal, oznacza to koniec ramki
      cntEndOfRxTick_SW = 0;
      posInRxTab_SW = 0;

      //OBLICZ SUME KONTROLNA
      uint8_t crcSumOnMCU_SW = HAL_CRC_Calculate(&hcrc, (uint32_t*)dataFromRx_SW, (RX_FRAME_LENGHT_SW - 2));

      //Sprawdz czy sumy kontrolne oraz bajt EOT (End Of Tranmission) sie zgadzaja
      if ( (dataFromRx_SW[RX_FRAME_LENGHT_SW - 2] == EOT_BYTE_SW) && (crcSumOnMCU_SW == dataFromRx_SW[RX_FRAME_LENGHT_SW - 1]) )
	{
	  processReceivedData_SW();
	  rs485_flt_SW = RS485_FLT_NONE_SW;
	  rejectedFramesInRow_SW = 0;
	}
      else
	{
	 // rejectedFramesInRow_SW++;
    	  processReceivedData_SW();
	  //Jezeli odrzucono wiecej niz 50 ramek z rzedu uznaj ze tranmisja zostala zerwana
	  if (rejectedFramesInRow_SW > 50)
	    {
	      resetActData_SW();
	      rs485_flt_SW = RS485_NEW_DATA_TIMEOUT_SW;
	    }
	}

      //Wyczysc bufor odbiorczy
      for (uint8_t i = 0; i < RX_FRAME_LENGHT_SW; i++)
	{
	  dataFromRx_SW[i] = 0x00;
	}

      __enable_irq();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//dopisac warunek ktory uart
  HAL_UART_Receive_DMA(&UART_PORT_RS485_SW, &RS485_BUFF_SW.rx, 3);			//Ponownie rozpocznij nasluchiwanie nasluchiwanie

  intRxCplt_SW = 1;								//Ustaw flage informujaca o otrzymaniu nowych danych

  if (posInRxTab_SW > RX_FRAME_LENGHT_SW) posInRxTab_SW = 0;				//Zabezpieczenie przed wyjsciem poza zakres tablicy

  dataFromRx_SW[posInRxTab_SW] = RS485_BUFF_SW.rx;					//Przypisz otrzymany bajt do analizowanej tablicy
  posInRxTab_SW++;

  HAL_UART_Receive_DMA(&UART_PORT_RS485_SW, &RS485_BUFF_EF.rx, 1);			//Ponownie rozpocznij nasluchiwanie nasluchiwanie

  intRxCplt_EF = 1;								//Ustaw flage informujaca o otrzymaniu nowych danych

  if (posInRxTab_EF > RX_FRAME_LENGHT_EF) posInRxTab_EF = 0;				//Zabezpieczenie przed wyjsciem poza zakres tablicy

  dataFromRx_EF[posInRxTab_EF] = RS485_BUFF_EF.rx;					//Przypisz otrzymany bajt do analizowanej tablicy
  posInRxTab_EF++;
}
/**
* @fn receiveData(void)
* @brief Funkcja ktorej zadaniem jest obsluga linii RX, umiescic wewnatrz rs485_step() przeplywu energii
*/
static void receiveData_EF(void)
{
  static uint32_t rejectedFramesInRow_EF;							//Zmienna przechowujaca liczbe straconych ramek z rzedu
  static uint32_t cntEndOfRxTick_EF;								//Zmienna wykorzystywana do odliczenia czasu wskazujacego na koniec transmisji

  //Sprawdz czy otrzymano nowe dane
  if (!intRxCplt_EF)
    {
      //Nie otrzymano nowych danych, zacznij odliczac czas
      cntEndOfRxTick_EF++;
    }
  else if (intRxCplt_EF)
    {
      //Nowe dane zostaly otrzymane, zeruj flage informujaca o zakonczeniu transmisji
      intRxCplt_EF = 0;
    }

  //Sprawdz czy minal juz czas wynoszacy RX_FRAME_LENGHT
  if (cntEndOfRxTick_EF > RX_FRAME_LENGHT_EF)
    {
      //Na czas przetwarzania danych wylacz przerwania
      __disable_irq();

      //Czas minal, oznacza to koniec ramki
      cntEndOfRxTick_EF = 0;
      posInRxTab_EF = 0;

      //OBLICZ SUME KONTROLNA
      uint8_t crcSumOnMCU_EF = HAL_CRC_Calculate(&hcrc, (uint32_t*)dataFromRx_EF, (RX_FRAME_LENGHT_EF - 2));

      //Sprawdz czy sumy kontrolne oraz bajt EOT (End Of Tranmission) sie zgadzaja
      if ( (dataFromRx_EF[RX_FRAME_LENGHT_EF - 2] == EOT_BYTE_EF) && (crcSumOnMCU_EF == dataFromRx_EF[RX_FRAME_LENGHT_EF - 1]) )
	{
	  processReceivedData_EF();
	  rs485_flt_EF = RS485_FLT_NONE_EF;
	  rejectedFramesInRow_EF = 0;
	}
      else
	{
	  //rejectedFramesInRow_EF++;
	  processReceivedData_EF();
	  //Jezeli odrzucono wiecej niz 50 ramek z rzedu uznaj ze tranmisja zostala zerwana
	  if (rejectedFramesInRow_EF > 50)
	    {
	      resetActData_EF();
	      rs485_flt_EF = RS485_NEW_DATA_TIMEOUT_EF;
	    }
	}

      //Wyczysc bufor odbiorczy
      for (uint8_t i = 0; i < RX_FRAME_LENGHT_EF; i++)
	{
	  dataFromRx_EF[i] = 0x00;
	}

      __enable_irq();
    }
}


/**
* @fn prepareNewDataToSend(void)
* @brief Funkcja przygotowujaca dane do wysylki, wykorzystana wewnatrz sendData(void) dla kierownicy
*/
static void prepareNewDataToSend_SW(void)
{
  uint8_t j = 0;

    RS485_TX_DATA_SW.interimSpeed = dataFromRx_SW[j];
    RS485_TX_DATA_SW.averageSpeed = dataFromRx_SW[++j];
  for (uint8_t k = 0; k < 2; k++)
      {
	    RS485_TX_DATA_SW.laptime_minutes.array[k] = dataFromRx_SW[++j];
      }

        RS485_TX_DATA_SW.laptime_seconds = dataFromRx_SW[++j];

    for (uint8_t k = 0; k < 2; k++)
      {
    	RS485_TX_DATA_SW.laptime_miliseconds.array[k] = dataFromRx_SW[++j];
      }
    for (uint8_t k = 0; k < 4; k++)
      {
    	RS485_TX_DATA_SW.FC_V.array[k] = dataFromRx_SW[++j];
      }

    for (uint8_t k = 0; k < 4; k++)
      {
    	RS485_TX_DATA_SW.FC_TEMP.array[k] = dataFromRx_SW[++j];
      }

    for (uint8_t k = 0; k < 2; k++)
      {
    	RS485_TX_DATA_SW.fcFanRPM.array[k] = dataFromRx_SW[++j];
      }

    for (uint8_t k = 0; k < 4; k++)
      {
    	RS485_TX_DATA_SW.CURRENT_SENSOR_FC_TO_SC.array[k] = dataFromRx_SW[++j];
      }

    for (uint8_t k = 0; k < 4; k++)
      {
    	RS485_TX_DATA_SW.CURRENT_SENSOR_SC_TO_MOTOR.array[k] = dataFromRx_SW[++j];
      }

    RS485_TX_DATA_SW.fcToScMosfetPWM = dataFromRx_SW[++j];
    RS485_TX_DATA_SW.motorPWM = dataFromRx_SW[++j];

    for (uint8_t k = 0; k < 4; k++)
      {
    	RS485_TX_DATA_SW.SC_V.array[k] = dataFromRx_SW[++j];
      }

    RS485_TX_DATA_SW.h2SensorDigitalPin = dataFromRx_SW[++j];
    RS485_TX_DATA_SW.emergencyButton = dataFromRx_SW[++j];

  //OBLICZ SUME KONTROLNA
  uint8_t calculatedCrcSumOnMCU_SW = HAL_CRC_Calculate(&hcrc, (uint32_t*)dataToTx_SW, (TX_FRAME_LENGHT_SW - 2) );

  //Wrzuc obliczona sume kontrolna na koniec wysylanej tablicy
  dataToTx_SW[TX_FRAME_LENGHT_SW - 1] = calculatedCrcSumOnMCU_SW;
}
/**
* @fn prepareNewDataToSend(void)
* @brief Funkcja przygotowujaca dane do wysylki, wykorzystana wewnatrz sendData(void) dla przeplywu energii
*/
static void prepareNewDataToSend_EF(void)
{
  uint8_t j = 0;

    RS485_TX_DATA_EF.fuellCellModeButtons = 2;
    RS485_TX_DATA_EF.scOn = 3;
    RS485_TX_DATA_EF.emergencyScenario = 4;
    RS485_TX_DATA_EF.motorPWM = dataFromRx_EF[++j];

  //OBLICZ SUME KONTROLNA
  uint8_t calculatedCrcSumOnMCU_EF = HAL_CRC_Calculate(&hcrc, (uint32_t*)dataToTx_EF, (TX_FRAME_LENGHT_EF - 2) );

  //Wrzuc obliczona sume kontrolna na koniec wysylanej tablicy
  dataToTx_EF[TX_FRAME_LENGHT_EF - 1] = calculatedCrcSumOnMCU_EF;
}
/**
* @fn processReveivedData_SW()
* @brief Funkcja przypisujaca odebrane dane do zmiennych docelowych dla kierownicy
*/
static void processReceivedData_SW(void)
{
  uint8_t i = 0;
  	  //Stany przyciskow
  	RS485_RX_VERIFIED_DATA_SW.halfGas = dataFromRx_SW[i];
  	RS485_RX_VERIFIED_DATA_SW.fullGas = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.horn = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.speedReset = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.powerSupply = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.scClose = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.fuelcellOff = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.fuelcellPrepareToRace = dataFromRx_SW[++i];
  	RS485_RX_VERIFIED_DATA_SW.fuelcellRace = dataFromRx_SW[++i];

  	dataFromRx_SW[++i] = EOT_BYTE_SW;
}
/**
* @fn processReveivedData_EF()
* @brief Funkcja przypisujaca odebrane dane do zmiennych docelowych dla przeplywu energii
*/
static void processReceivedData_EF(void)
{
  uint8_t i = 0;

  for(uint8_t k = 0; k < 4; k++)
  {
  	RS485_RX_VERIFIED_DATA_EF.FC_V.array[k] = dataFromRx_EF[++i];
  }
  for(uint8_t k = 0; k < 4; k++)
  {
  	RS485_RX_VERIFIED_DATA_EF.FC_TEMP.array[k] = dataFromRx_EF[++i];
  }
  for(uint8_t k = 0; k < 4; k++)
  {
  	RS485_RX_VERIFIED_DATA_EF.CURRENT_SENSOR_FC_TO_SC.array[k] = dataFromRx_EF[++i];
  }
  for(uint8_t k = 0; k < 4; k++)
  {
  	RS485_RX_VERIFIED_DATA_EF.SC_V.array[k] = dataFromRx_EF[++i];
  }
  for(uint8_t k = 0; k < 2; k++)
  {
  	RS485_RX_VERIFIED_DATA_EF.fcFanRPM.array[k] = dataFromRx_EF[++i];
  }
  //	RS485_RX_VERIFIED_DATA_EF.fcToScMosfetPWM = dataFromRx_EF[++i];
  	RS485_RX_VERIFIED_DATA_EF.emergency = dataFromRx_EF[++i];

  	dataFromRx_SW[++i] = EOT_BYTE_EF;
}
/**
* @fn resetActData_SW
* @brief Zerowanie zmiennych docelowych (odbywa sie m.in w przypadku zerwania transmisji) dla kierownicy
*/
static void resetActData_SW(void)
{
  RS485_RX_VERIFIED_DATA_SW.halfGas = 0;
  RS485_RX_VERIFIED_DATA_SW.fullGas = 0;
  RS485_RX_VERIFIED_DATA_SW.horn = 0;

  RS485_RX_VERIFIED_DATA_SW.speedReset = 0;
  RS485_RX_VERIFIED_DATA_SW.powerSupply = 0;

  RS485_RX_VERIFIED_DATA_SW.scClose = 0;
  RS485_RX_VERIFIED_DATA_SW.fuelcellOff = 0;

  RS485_RX_VERIFIED_DATA_SW.fuelcellPrepareToRace = 0;
  RS485_RX_VERIFIED_DATA_SW.fuelcellRace = 0;
}
/**
* @fn resetActData_EF
* @brief Zerowanie zmiennych docelowych (odbywa sie m.in w przypadku zerwania transmisji) dla przeplywu energii
*/
static void resetActData_EF(void)
{

  for(uint8_t k = 0; k < 4; k++)
  	{
	  RS485_RX_VERIFIED_DATA_EF.FC_V.array[k] = 0;
    }

  for (uint8_t k = 0; k < 4; k++)
    {
      RS485_RX_VERIFIED_DATA_EF.FC_TEMP.array[k] = 0;
    }
  for (uint8_t k = 0; k < 4; k++)
    {
      RS485_RX_VERIFIED_DATA_EF.CURRENT_SENSOR_FC_TO_SC.array[k] = 0;
    }
  for (uint8_t k = 0; k < 4; k++)
    {
      RS485_RX_VERIFIED_DATA_EF.SC_V.array[k] = 0;
    }
  for (uint8_t k = 0; k < 2; k++)
    {
      RS485_RX_VERIFIED_DATA_EF.fcFanRPM.array[k] = 0;
    }
  RS485_RX_VERIFIED_DATA_EF.emergency = 0;
}
