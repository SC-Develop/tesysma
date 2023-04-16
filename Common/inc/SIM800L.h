/*
 * SIM800L.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef SRC_SIM800L_H_
#define SRC_SIM800L_H_

#include "SIM800L.def.h"

#define MAX_AT_LENGTH           64  // lunghezza massima di un comando AT
#define MAX_SMS_LENGTH          512 // lunghezza massima di un SMS

typedef enum {

	PHONE_1 = 1,
	PHONE_2 = 2,
	PHONE_3 = 3,
    PHONE_MAX,

} PhonebookIdEntry_TypeDef;

typedef enum {

	GSM_WAITING_FOR_READY,
	GSM_WAITING_FOR_IDLE,
	GSM_IDLE,
	GSM_SEND_AT_COMMAND,
	GSM_WAITING_FOR_REPLY,
	GSM_CALL_IN_PROGRESS,
	GSM_CALL_ANSWERED,
	GSM_ERROR,

} GSMStatus_TypeDef;

/**
 * @struct
 * @brief
 *
 */
typedef struct
{
	PhonebookIdEntry_TypeDef entry;
	char number[MAX_NUM_LENGTH];
} PhonebookEntry_TypeDef;

/**
 * @struct
 * @brief
 *
 */
typedef struct {

	char num[MAX_NUM_LENGTH];
	// char at_cmd[MAX_AT_LENGTH];
	char mess[MAX_SMS_LENGTH];

} SMS_TypeDef;

void SIM800L_Init(void);
void SIM800L_HangUp(void);
void SIM800L_SM_Exec(void);
void SIM800L_SetClipNumber(char *number);
char *SIM800L_GetClipNumber(void);
void SIMM800L_SMS2PhoneNumber(char *num, char *mess);

uint8_t SIMM800L_Schedule_AddPhonebookEntry(char * num, PhonebookIdEntry_TypeDef entry);
uint8_t SIMM800L_Schedule_DelPhonebookEntry(PhonebookIdEntry_TypeDef entry);
void SIM800L_Schedule_Call_Phonebook_Entries(void);
void SIMM800L_Schedule_SMS(SMS_TypeDef *sms);
void SIMM800L_Schedule_SMS_Get_Params(SMS_TypeDef *sms);
PhonebookEntry_TypeDef *GetPhonebook(void);
uint8_t SetPhonebookEntry(char *num, PhonebookIdEntry_TypeDef entry);
uint8_t SIMM800L_CallPhonebookEntry(PhonebookIdEntry_TypeDef entry);

void SetBattCharge(uint8_t perc);
void SetVBatt(float volt);
uint8_t GSM_Calling(void);
GSMStatus_TypeDef GSM_Status(void);

#endif /* SRC_SIM800L_H_ */
