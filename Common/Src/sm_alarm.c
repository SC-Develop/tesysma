/**
 * @file sm_alarm.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "main.h"
#include "adc.h"
#include "stdbool.h"
#include "ntc.h"
#include "timsys.h"
#include "sm_alarm.h"
#include "sm_adc.h"
#include "SIM800L.h"

#define PANIC_TIMEOUT 300000 // 5 min.

/**
 * @struct
 * @brief State Machine of Temperature Alarm
 *
 */
typedef struct {

	float    temp_threshold;
	float    temp;

	uint8_t  autoEnable;

	AlarmStatus_TypeDef  alarm;

	AlarmSMStatus_TypeDef status;

} AlarmStateMachine_TypeDef;

// Variables ------------------------------------------------------------------------------------------------------------------------------

static AlarmStateMachine_TypeDef alarm_sm = {
	.alarm 			= ALARM_ON,
	.autoEnable 	= 1,
	.temp 			= 0,
	.temp_threshold = 100,
	.status 		= AS_IDLE,
};

// - Exported functions -------------------------------------------------------------------------------------------------------------------

/**
 * @fn void SetAlarm(uint8_t)
 * @brief
 *
 * @param val
 */
void EnableAlarm(AlarmStatus_TypeDef status)
{
	alarm_sm.alarm = status;
}

/**
 * @fn void SetAlamAutoEnable(uint8_t)
 * @brief
 *
 * @param enabled
 */
void SetAlarmAutoEnable(uint8_t enabled)
{
	alarm_sm.autoEnable = enabled;
}

/**
 * @fn void SetAlamAutoEnable(uint8_t)
 * @brief
 *
 * @param enabled
 */
uint8_t AlarmAutoEnable(void)
{
	return alarm_sm.autoEnable;
}

/**
 * @fn AlarmStatus_TypeDef AlarmStatus(void)
 * @brief
 *
 * @return
 */
AlarmStatus_TypeDef AlarmStatus(void)
{
	return alarm_sm.alarm;
}

/**
 * @fn float GetTemp(void)
 * @brief
 *
 * @return
 */
float GetTemp(void)
{
	return alarm_sm.temp;
}

/**
 * @fn void SetTemp(float)
 * @brief
 *
 * @param t
 */
void SetTemp(float temp)
{
	alarm_sm.temp = temp;
}

/**
 * @fn void SetTempThreshold(float)
 * @brief
 *
 * @param temp
 */
void SetTempThreshold(float temp)
{
	alarm_sm.temp_threshold = temp;

	union {
		float    t;
		uint8_t  data[4];
	} t;

	t.t = temp;

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef EraseInit = {
		.TypeErase    = FLASH_TYPEERASE_SECTORS,
		.Banks        = FLASH_BANK_1,
		.VoltageRange = FLASH_VOLTAGE_RANGE_3,
		.Sector       = FLASH_SECTOR_5 ,
		.NbSectors    = 1};

	uint32_t SectorError;

	if (HAL_FLASHEx_Erase(&EraseInit, &SectorError) == HAL_OK)
	{
		for (int8_t n = 0; n < 4; n++)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, DATA_CFG_ADDRESS + n, t.data[n]);
		}
	}
	else
	{
		// error
	}

	HAL_FLASH_Lock();
}

/**
 * @fn float GetTempThreshold(void)
 * @brief
 *
 * @return
 */
float GetTempThreshold(void)
{
	return alarm_sm.temp_threshold;
}

/**
 * @fn float TemperatureSampling(void)
 * @brief esegue la macchina astati del campionamento della temperatura
 *
 * @return
 */
float TemperatureSampling_Exec(void)
{
	if (ADCInterface()->isStopped())            // se l'adc è fermo
	{
		ADCInterface()->Start(); // fa partire l'ADC
	}
	else // ADC Started
	{
		ADCInterface()->Exec();

		float ntcTension = ADCInterface()->ChannelValue(CHN_NTC_TEMP);

		if (ntcTension != 0x0FFFF) 					    // se l'acquisizione è completata
		{
			alarm_sm.temp = NTC_Temp(NTC1, ntcTension); // Calcola ed imposta la temperatura

			ADCInterface()->Stop(); 				    // ferma l'ADC

			return alarm_sm.temp;
		}
	}

	return 0xFFFF;
}

/**
 * @fn float ReadTemperature(void)
 * @brief Esegue la macchina a stati per la lettura della temperatura per un massimo di 2 sec.
 *
 * @return la temperatura in °C oppure 0xFFFF in caso di timeout
 */
float ReadTemperature(void)
{
	uint32_t time = HAL_GetTick() + 2000;

	float temp;

	while (HAL_GetTick() < time) // wait almost 1000 msec for temperature sampling
	{
       temp = TemperatureSampling_Exec();

       if (temp != 0xFFFF)
       {
    	   break;
       }
	}

	return temp;
}

/**
 * @fn void SM_Alarm_Init(void)
 * @brief
 *
 */
void SM_Alarm_Init(void)
{
	ADCInterface()->Init(&hadc1,ADC_VDD);
}

/**
 * @fn void Alarm_SM_Exec(void)
 * @brief
 *
 */
void SM_Alarm_Exec(void)
{
	static uint32_t timeout = 0; // scadenza immediata
	static uint32_t time    = 0; //

	float temp;

	switch(alarm_sm.status)
	{
		case AS_IDLE:

			if (TimSys_TickTimeElapsed(&time, timeout)) // controlla temperatura solo allo scadere del timeout solo se non c'è alcuna chiamata in uscita
			{
				if (GSM_Status() == GSM_IDLE)
				{
					alarm_sm.status = AS_CHECK_TEMPERATURE;
				}

				timeout = TEMPERATURE_SAMPLING_TIME;    // reimposta il timeout
			}

		break;

		case AS_CHECK_TEMPERATURE:

			temp = TemperatureSampling_Exec();

			if (temp != 0xFFFF)
			{
				if ( (alarm_sm.alarm == ALARM_ON ) && (alarm_sm.temp < alarm_sm.temp_threshold) )
				{
					SIM800L_Schedule_Call_Phonebook_Entries(); // Schedula la chiamata di allarme a tutti i numeri

					alarm_sm.status = AS_CALLING;              // Cambia lo stato della macchina

					time = HAL_GetTick();

					return;
				}
				else
				{
					if (alarm_sm.temp > (alarm_sm.temp_threshold + TEMPERATURE_DELTA_THRESHOLD) && alarm_sm.autoEnable)
					{
						alarm_sm.alarm = ALARM_ON; // reset alarm
					}
				}

				alarm_sm.status = AS_IDLE;
			}

		break;

		case AS_CALLING: // waiting for terminating alarm calling

			if (TimSys_TickTimeElapsed(&time, PANIC_TIMEOUT))
			{
				HAL_NVIC_SystemReset();
			}

			if ( (GSM_Calling() >= PHONE_3) && (GSM_Status() == GSM_IDLE) ) // when the last call was hang up
			{
				time = HAL_GetTick();

				alarm_sm.status = AS_IDLE;
			}

		break;
	}
}



