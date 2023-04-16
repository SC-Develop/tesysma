/**
 * @file   ac_app.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "main.h"
#include "def.h"
#include "iwdg.h"
#include "usart.h"
#include "stm32_lib_usart.h"
#include "ac_app.h"
#include "parser.h"
#include "sm_alarm.h"
#include "SIM800L.h"

// Local functions -----------------------------------------------------------------------------------------------------------------------

static char *version = AC_VERSION; // AC_VERSION È UNA DEFINE CHE PUNTA AD UNA VARIABILE DI AMBIENTE STRINGA, DEFINITA NELLE PROPRIETÀ DEL PROGETTO
static char build_date[11];

/**
 * @fn void App_Init(void)
 * @brief
 *
 */
static void App_Init(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);   // leave to reset (OFF) SIM800L Module

	USART_Init();

	USART_SetHandle(USART_1, &huart1);
	USART_SetHandle(USART_2, &huart2);

	USART_Start(USART_1);
	USART_Start(USART_2);

	sprintf(build_date,"20%d-%02d-%02d", AC_VERSION_YEAR, AC_VERSION_MONTH, AC_VERSION_DAY);

	USART_Printf(USART_2, "\r\n\r\nSCD TESYS-MA %s - %s\r\n", version, build_date);
	USART_Printf(USART_2, "\r\nTemperature computing...\r\n");

	// delay the starting to avoid false starting due to power spark instability that can can cause the flash memory writing error on calling SetTempThreshold() function

	HAL_Delay(5000);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);   // start SIM800L

	WatchdogRefresh();

	SM_Alarm_Init();

	ParserInit();

	SIM800L_Init();

	float temp = *(float*) DATA_CFG_ADDRESS;

	if (temp >-50 && temp <=100)
	{
		SetTempThreshold(*(float*) DATA_CFG_ADDRESS);
	}

	SetTemp(ReadTemperature()); // waiting for temperature reading

	USART_Printf(USART_2, "\r\nTemperature: %0.1f °C\r\nThreshold  : %0.1f °C\r\n", GetTemp(), GetTempThreshold());
}

// Exported functions -----------------------------------------------------------------------------------------------------------------------

/**
 * @fn void WatchDogRefresh(void)
 * @brief
 *
 */
void WatchdogRefresh(void)
{
  #ifdef WATCHDOG

	HAL_IWDG_Refresh(&hiwdg);

  #endif
}

/**
 * @fn void App_Start(void)
 * @brief
 *
 */
void App_Start(void)
{
	App_Init();

	while (1)
	{
		SIM800L_SM_Exec();

		SM_Alarm_Exec();
	}
}

/**
 * @fn char App_Version*(void)
 * @brief
 *
 * @return
 */
char* App_Version(void)
{
	return version;
}

/**
 * @fn char App_BuildDate*(void)
 * @brief
 *
 * @return
 */
char *App_BuildDate(void)
{
	return build_date;
}
