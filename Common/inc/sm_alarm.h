/*
 * sm_alarm.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef INC_SM_ALARM_H_
#define INC_SM_ALARM_H_

// Defines ----------------------------------------------------------------------

#define DATA_CFG_ADDRESS          	0X08020000  // 0x0803FC00
#define TEMPERATURE_SAMPLING_TIME 	60000
#define TEMPERATURE_DELTA_THRESHOLD 1.0

// Types definition ---------------------------------------------------------------

/**
 * @enum
 * @brief
 *
 */
typedef enum {
	ALARM_OFF = 0,/**< ALARM_OFF */
	ALARM_ON  = 1,/**< ALARM_ON */
} AlarmStatus_TypeDef;

/**
 * @enum
 * @brief
 *
 */
typedef enum {

	AS_IDLE,             /**< AS_IDLE */
	AS_CHECK_TEMPERATURE,/**< AS_CHECK_TEMPERATURE */
	AS_CALLING,          /**< AS_CALLING */

} AlarmSMStatus_TypeDef;

// exported functions prototype ---------------------------------------------------

float GetTemp(void);
void  SetTemp(float temp);
void  SetTempThreshold(float temp);
float GetTempThreshold(void);
void  EnableAlarm(uint8_t val);
void  SetAlarmAutoEnable(uint8_t enabled);

uint8_t AlarmAutoEnable(void);

float ReadTemperature(void);

void SM_Alarm_Exec(void);
void SM_Alarm_Init(void);

AlarmStatus_TypeDef AlarmStatus(void);

#endif /* INC_SM_ALARM_H_ */
