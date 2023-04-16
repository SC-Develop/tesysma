/**
 * @file   SIM800L.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "main.h"
#include "stdlib.h"
#include "ac_app.h"
#include "sm_alarm.h"
#include "SIM800L.h"
#include "stm32_lib_usart.h"
#include "parser.h"
#include "timsys.h"

#define MAX_SCHEDULER 			32  	// Si possono schedulare un massimo di 32 comandi consecutivi
#define MAX_PHONEBOOK_ENTRY 	3   	// NON CAMBIARE !!! IN CASO CONTRARIO MODIFICARE IL COMANDO => [AT_READ_PHONEBOOK] = "AT+CPBR=1,3\n",
#define AT_DELAY                200 	// [msec] attesa dopo il completamento di un comando AT
#define GSM_STARTING_DELAY      1000

typedef enum {

	AT_WRITE_PHONEBOOK_ENTRY = 0,
	AT_DEL_PHONEBOOK_ENTRY,
	AT_READ_PHONEBOOK,
	AT_CALL_PHONEBOOK_ENTRY,
	AT_CALL_PHONENUMBER,
	AT_SMS_READ_ENTRY,
	AT_SMS_DEL_ENTRY,
	AT_SMS,
	AT_SMS_GET_PARAMS,
	AT_HANG_UP,
	AT_ANSWER,
	AT_DTMF_ENABLE,
	AT_SMS_TEXT_MODE,
	AT_DTMF_SHARP,
	AT_DTMF_STAR,
	AT_DTMF_DURATION,
	AT_CLIP,
	AT_CBC,
	AT_NOECHO,
	AT_CGATT,
	AT_MORING,
	AT_CALM,
	AT_MUT,
	AT_CRSL,
	AT_LVL,
	AT_CMIC0,
	AT_CMIC1,
	AT_CMIC2,
	AT_CMIC3,
	AT_AT,
	AT_WELCOME_SMS,
	AT_IMEI,
	AT_COPS,
	AT_CSQ,
	AT_SMSDEL,
    AT_MAX_ID,

} ATCommand_ID_TypeDef;

/**
 * @struct
 * @brief
 *
 */
typedef struct {

  ATCommand_ID_TypeDef id;
  void *data;

} ATCommandData_TypeDef;

/**
 * @struct
 * @brief
 *
 */
typedef struct {

	char    imei[16];
	char    operator[64];
	char    clipnumber[MAX_NUM_LENGTH];

	uint8_t signal;		// gsm signal strength
	uint8_t call_entry;	// phonebook entry of outgoing call
    uint8_t ring;
    uint8_t battCharge;

    float   vbatt;

	int8_t fifo_items;  // scheduler items
	int8_t fifo_head;

	SMS_TypeDef sms[MAX_PHONEBOOK_ENTRY];

	char at_cmd[MAX_AT_LENGTH];

	ATCommandData_TypeDef  scheduler[MAX_SCHEDULER];

	PhonebookEntry_TypeDef phonebook[MAX_PHONEBOOK_ENTRY];

	ATCommand_ID_TypeDef   command; // last at command id sent

	GSMStatus_TypeDef      status;

} GSM_TypeDef;

// - Local Variables ----------------------------------------------------------------- /

static char ctrlz = 0x1A;

static char sms[MAX_SMS_LENGTH + 1];

static char *atcommands[AT_MAX_ID] = {

	[AT_WRITE_PHONEBOOK_ENTRY] = NULL,
	[AT_DEL_PHONEBOOK_ENTRY	 ] = NULL,
	[AT_READ_PHONEBOOK 		 ] = "AT+CPBR=1,3\n",
	[AT_CALL_PHONEBOOK_ENTRY ] = NULL,
	[AT_CALL_PHONENUMBER     ] = NULL,
	[AT_SMS_READ_ENTRY		 ] = NULL,
	[AT_SMS_DEL_ENTRY    	 ] = NULL,
	[AT_SMS     			 ] = NULL,
	[AT_SMS_GET_PARAMS		 ] = NULL,
	[AT_HANG_UP 			 ] = "ATH\n",
	[AT_ANSWER 			     ] = "ATA\n",
	[AT_DTMF_ENABLE			 ] = "AT+DDET=1\n",
	[AT_SMS_TEXT_MODE	     ] = "AT+CMGF=1\n",
	[AT_DTMF_SHARP	         ] = "AT+VTS=\"#\"\n",
	[AT_DTMF_STAR            ] = "AT+VTS=\"*,*,*\"\n",
	[AT_DTMF_DURATION        ] = "AT+VTD=10\n",
	[AT_CLIP                 ] = "AT+CLIP=1\n",
	[AT_CBC                  ] = "AT+CBC\n",
	[AT_NOECHO               ] = "ATE0\n",
	[AT_CGATT                ] = "AT+CGATT=0\n",
	[AT_MORING				 ] = "AT+MORING=0\n",  	// OUTGOING CALLS UNSOLICITED RING/CONNECTED MESSAGE
	[AT_CALM				 ] = "AT+CALM=1\n",    	// ALARM SOUND OFF
	[AT_MUT					 ] = "AT+CMUT=1\n",    	// MUTE ON (NON FUNZIONA)
	[AT_CRSL				 ] = "AT+CRSL=0\n",     // RING LEVEL TO 0
	[AT_LVL					 ] = "AT+CLVL=0\n",   	// LOUDSPEAKER SOUND LEVEL TO 0
	[AT_CMIC0     			 ] = "AT+CMIC=0,0\n",   // LOUDSPEAKER SOUND LEVEL TO 0
	[AT_CMIC1     			 ] = "AT+CMIC=1,0\n",   // LOUDSPEAKER SOUND LEVEL TO 0
	[AT_CMIC2     			 ] = "AT+CMIC=2,0\n",   // LOUDSPEAKER SOUND LEVEL TO 0
	[AT_CMIC3    			 ] = "AT+CMIC=3,0\n",   // LOUDSPEAKER SOUND LEVEL TO 0
	[AT_AT    	     		 ] = "AT\n",            // LOUDSPEAKER SOUND LEVEL TO 0
	[AT_WELCOME_SMS          ] = NULL,
	[AT_IMEI                 ] = "AT+CGSN\n",
	[AT_COPS                 ] = "AT+COPS?\n",
	[AT_CSQ                  ] = "AT+CSQ\n",
	[AT_SMSDEL               ] = "AT+CMGD=1,4\n",

};

static GSM_TypeDef gsm = {.fifo_items = 0, .fifo_head = 0, .status = GSM_WAITING_FOR_READY, .call_entry = 0, .ring = 0, };

/**
 * @fn void SetBattCharge(uint8_t)
 * @brief
 *
 * @param perc
 */
void SetBattCharge(uint8_t perc)
{
	gsm.battCharge = perc;
}

/**
 * @fn void SetVBatt(float)
 * @brief
 *
 * @param volt
 */
void SetVBatt(float volt)
{
	gsm.vbatt = volt;
}

/**
 * @fn GSMStatus_TypeDef GSM_Status(void)
 * @brief
 *
 * @return
 */
GSMStatus_TypeDef GSM_Status(void)
{
	return gsm.status;
}

/**
 * @fn uint8_t GSN_Calling(void)
 * @brief
 *
 * @return
 */
uint8_t GSM_Calling(void)
{
	return gsm.call_entry;
}

/**
 * @fn PhonebookEntry_TypeDef GetPhonebook*(void)
 * @brief
 *
 * @return
 */
PhonebookEntry_TypeDef *GetPhonebook(void)
{
	return gsm.phonebook;
}

/**
 * @fn uint8_t SetPhonebookEntry(char*, PhonebookIdEntry_TypeDef)
 * @brief
 *
 * @param num
 * @param entry
 * @return
 */
uint8_t SetPhonebookEntry(char *num, PhonebookIdEntry_TypeDef entry)
{
	if (entry > 0 &&  entry < PHONE_MAX)
	{
		uint8_t i = entry - 1;

		memset(gsm.phonebook[i].number, 0, MAX_NUM_LENGTH);	// Azzera la struttura dati

		strncpy(gsm.phonebook[i].number, num, strlen(num));  // Copia il numero nella rubrica

		gsm.phonebook[i].entry  = entry;                     // Imposta l'indice della ribbrica

		return 1;
	}

	return 0;
}

/**
 * @fn int8_t Scheduler_Push(ATScheduler_TypeDef*)
 * @brief
 *WRITE_PHONEBOOK_ENTRY;
 * @param item
 * @return
 */
static uint8_t SIM800L_Scheduler_Push(ATCommandData_TypeDef *item)
{
   if (gsm.fifo_items < MAX_SCHEDULER) // se lo scheduler non è pieno
   {
	   gsm.scheduler[gsm.fifo_items++] = *item; // copy data into scheduler item

	   return 1;
   }

   return 0; // fifo full
}

/**
 * @fn int8_t Scheduler_Push(ATScheduler_TypeDef*)
 * @brief
 *
 * @param item
 * @return
 */
static int8_t SIM800L_Scheduler_Pop(ATCommandData_TypeDef *item)
{
	if (gsm.fifo_items > 0) // se lo scheduler non è vuoto
	{
		//*item = gsm.scheduler[--gsm.fifo_items];

		*item = gsm.scheduler[gsm.fifo_head++];

		if (gsm.fifo_head == gsm.fifo_items)
		{
			gsm.fifo_head  = 0;
			gsm.fifo_items = 0;
		}

		return 1;
	}

	return 0; // empty fifo
}

/**
 * @fn void SIM800L_Init(void)
 * @brief
 *
 */
void SIM800L_Init(void)
{
	ATCommand_ID_TypeDef init[] = {
		AT_CLIP,
		AT_SMS_TEXT_MODE,
		AT_DTMF_ENABLE,
		AT_DTMF_DURATION,
		AT_MORING,
		AT_CALM,
		AT_CRSL,
		AT_LVL,
		AT_CMIC0,
		AT_CMIC1,
		AT_CMIC2,
		AT_CMIC3,
		AT_CBC,
		AT_READ_PHONEBOOK,
		AT_IMEI,
		AT_COPS,
		AT_CGATT,
		AT_CSQ,
		AT_SMSDEL,
		// AT_NOECHO,
		AT_WELCOME_SMS,
	};

	ATCommandData_TypeDef command;

	memset(&gsm.phonebook,0,sizeof(gsm.phonebook)); // reset phonebook list

	command.data = NULL;

	uint8_t items = sizeof(init)/sizeof(init[0]);

	for (uint8_t n = 0; n < items; n++)
	{
		command.id = init[n];

		SIM800L_Scheduler_Push(&command);
	}

	gsm.status = GSM_WAITING_FOR_READY;
}

/**
 * @fn void SIM800L_SentClipNumber(char*)
 * @brief
 *
 * @param number
 */
void SIM800L_SetClipNumber(char *number)
{
	if (strlen(number)<15)
	{
		strcpy(gsm.clipnumber,number);
	}
}

/**
 * @fn char SIM800L_GetClipNumber*(void)
 * @brief
 *
 * @return
 */
char *SIM800L_GetClipNumber(void)
{
	return gsm.clipnumber;
}

/**
 * @fn void SIM800L_SendATCommand(char*)
 * @brief
 *
 * @param cmd
 */
void SIM800L_SendATCommand(ATCommand_ID_TypeDef id)
{
	gsm.command = id;
	gsm.status  = GSM_SEND_AT_COMMAND;
}

/**
 * @fn void SIM800L_HangUp(void)
 * @brief Invia il comando AT di HANGUP direttamente sulla seriale. Lo stato della macchina a stati non viene modificato. Rimane nello stato corrente.
 *        Quindi lo stato corrente deve farsi carico di rilevare la risposta al comando (OK).
 *
 */
void SIM800L_HangUp(void)
{
	gsm.command = AT_HANG_UP;

	USART_Write(USART_1, atcommands[AT_HANG_UP], 0);
}

/**
 * @fn void SIM800L_Answer(void)
 * @brief
 *
 */
void SIM800L_Answer(void)
{
	SIM800L_SendATCommand(AT_ANSWER);
}

/**
 * @fn void SIMM800L_AddPhoneEntry(PhoneNumberEntry_TypeDef)
 * @brief
 *
 * @param number
 */
void SIMM800L_AddPhonebookEntry(PhonebookEntry_TypeDef *number)
{
	atcommands[AT_WRITE_PHONEBOOK_ENTRY] = gsm.at_cmd;

	sprintf(atcommands[AT_WRITE_PHONEBOOK_ENTRY],"AT+CPBW=%d,\"%s\"\n", number->entry, number->number);

	SIM800L_SendATCommand(AT_WRITE_PHONEBOOK_ENTRY);
}

/**
 * @fn void SIMM800L_DelPhoneEntry(uint8_t)
 * @brief
 *
 * @param index
 */
void SIMM800L_DelPhonebookEntry(PhonebookIdEntry_TypeDef entry)
{
	if (entry > 0 && entry < PHONE_MAX)
	{
		atcommands[AT_DEL_PHONEBOOK_ENTRY] = gsm.at_cmd;

		memset(gsm.phonebook[entry-1].number,0,sizeof(gsm.phonebook[entry-1].number));

		gsm.phonebook[entry-1].entry = 0;

		sprintf(atcommands[AT_DEL_PHONEBOOK_ENTRY],"AT+CPBW=%d\n", entry);

		SIM800L_SendATCommand(AT_DEL_PHONEBOOK_ENTRY);
	}
}

/**
 * @fn void SIMM800L_CallPhonebookEntry(uint8_t)
 * @brief
 *
 * @param index
 */
uint8_t SIMM800L_CallPhonebookEntry(PhonebookIdEntry_TypeDef entry)
{
	if (entry > 0 && entry < PHONE_MAX)
	{
		if (gsm.status == GSM_IDLE)
		{
			gsm.call_entry = entry; // save current phone number entry

			if (strlen(gsm.phonebook[entry-1].number))
			{
				sprintf(gsm.at_cmd,"atd%s;\n",gsm.phonebook[entry-1].number);

				atcommands[AT_CALL_PHONEBOOK_ENTRY] = gsm.at_cmd;

				SIM800L_SendATCommand(AT_CALL_PHONEBOOK_ENTRY);
			}

			return 1; // restituisce successo anche se non c'è il numero
		}
	}

	return 0;
}

/**
 * @fn void SIMM800L_AddPhoneNumber(char*, PhonebookEntry_TypeDef)
 * @brief schedule the "add phone number" operation
 *
 * @param num
 * @param entry (1->phonemax)
 */
uint8_t SIMM800L_Schedule_AddPhonebookEntry(char * num, PhonebookIdEntry_TypeDef entry)
{
	if (entry > 0 && entry < PHONE_MAX)
	{
		SetPhonebookEntry(num, entry);

        ATCommandData_TypeDef command = {.id = AT_WRITE_PHONEBOOK_ENTRY, .data = (void *) &gsm.phonebook[entry-1]}; // Imposta il record dati per lo scheduler

        SIM800L_Scheduler_Push(&command); // schedule

        return 1;
	}

	return 0;
}

/**
 * @fn void SIMM800L_DelPhoneEntry(uint8_t)
 * @brief
 *
 * @param index
 */
uint8_t SIMM800L_Schedule_DelPhonebookEntry(PhonebookIdEntry_TypeDef entry)
{
	if (entry > 0 && entry < PHONE_MAX)
	{
		gsm.phonebook[entry-1].entry  = entry;                       // Imposta l'indice della ribbrica

		ATCommandData_TypeDef command = {.id = AT_DEL_PHONEBOOK_ENTRY, .data = (void*) &gsm.phonebook[entry-1].entry};

		SIM800L_Scheduler_Push(&command); // schedule

		return 1;
	}

	return 0;
}

/**
 * @fn void SIM800L_Schedule_Call_Phonebook_Entries(void)
 * @brief Schedule the the sequence of call to all entries of phonebook
 *
 */
void SIM800L_Schedule_Call_Phonebook_Entries(void)
{
	// gsm.call_entry = 0; // Alarm call entry

	for (uint8_t n = PHONE_1; n < PHONE_MAX; n++)
	{
		gsm.phonebook[n-1].entry = n;

		ATCommandData_TypeDef command = {.id = AT_CALL_PHONEBOOK_ENTRY, .data = (void*) &gsm.phonebook[n-1].entry};

		SIM800L_Scheduler_Push(&command); // schedule
	}
}

/**
 * @fn void SIMM800L_Schedule_SMS(char*, char*)
 * @brief
 *
 * @param num
 * @param mess
 */
/*void SIMM800L_Schedule_SMS(char *num, char *mess)
{
	if (strlen(num) < 16 && strlen(mess) < MAX_SMS_LENGTH)
	{
		strcpy(gsm.sms[0].mess, mess);  // Assembla il messaggio
		strcpy(gsm.sms[0].num , num );	 // Copia il numero

		ATCommandData_TypeDef command = {.id = AT_SMS, .data = (void *) &gsm.sms};

		SIM800L_Scheduler_Push(&command); // schedule
	}
}*/

/**
 * @fn void SIMM800L_Schedule_SMS(SMS_TypeDef*)
 * @brief
 *
 * @param sms
 */
void SIMM800L_Schedule_SMS(SMS_TypeDef *sms)
{
	if (strlen(sms->num) < 16 && strlen(sms->mess) < MAX_SMS_LENGTH)
	{
		// strcpy(gsm.sms[0].mess, mess);  // Assembla il messaggio
		// strcpy(gsm.sms[0].num , num );	 // Copia il numero

		ATCommandData_TypeDef command = {.id = AT_SMS, .data = (void *) &gsm.sms};

		SIM800L_Scheduler_Push(&command); // schedule
	}
}


/**
 * @fn void SIMM800L_Schedule_SMS(char*, char*)
 * @brief schedula l'invio di un messaggio con la lettura dei parametri correnti al numero 'num'
 *
 * @param num
 * @param mess
 */
/*void SIMM800L_Schedule_SMS_Get_Params(char *num)
{
	if (strlen(num) < MAX_NUM_LENGTH)
	{
		memset(gsm.sms.mess, 0, sizeof(gsm.sms.mess));  // cancella l'ultima stringa inviata
		strcpy(gsm.sms.num , num );	                    // Copia il numero

		ATCommandData_TypeDef command = {.id = AT_SMS_GET_PARAMS, .data = (void *) &gsm.sms};

		SIM800L_Scheduler_Push(&command); // schedule
	}
}*/

/**
 * @fn void SIMM800L_Schedule_SMS_Get_Params(SMS_TypeDef*)
 * @brief
 *
 * @param sms
 */
void SIMM800L_Schedule_SMS_Get_Params(SMS_TypeDef *sms)
{
	if (strlen(sms->num) < MAX_NUM_LENGTH)
	{
		memset(sms->mess, 0, sizeof(sms->mess));  // cancella l'ultima stringa inviata

		ATCommandData_TypeDef command = {.id = AT_SMS_GET_PARAMS, .data = (void *) sms};

		SIM800L_Scheduler_Push(&command); // schedule
	}
}

/**
 * @fn void SIMM800L_SMS2PhoneNumber(char*, char*)
 * @brief Invia un SMS: Il comando può essere eseguito solo se no vi sono chiamate in corso e non cisono altri comandi in esecuzione,
 *        cioè solamente quando la macchiona astati è in IDLE
 *
 * @param num
 * @param mess
 */
int8_t SIMM800L_SMS(SMS_TypeDef *psms)
{
	static char at_cmd[MAX_AT_LENGTH];

	if (gsm.status != GSM_IDLE)
	{
		return -1; // command refused
	}

	if (strlen(psms->num) < 16 && strlen(psms->mess) < MAX_SMS_LENGTH)
	{
		sprintf(sms, "%s%c", psms->mess, ctrlz); 			  // aggiunge il carattere terminatore al messaggio e salva i dati nella struttura apposita

		sprintf(at_cmd,"%s\"%s\"\r\n","AT+CMGS=",psms->num);  // assembla la stringa di comando

		atcommands[AT_SMS] = at_cmd;   			   	          // set command string pointer

		SIM800L_SendATCommand(AT_SMS);

		return 1;
	}

	return 0; // data overflow
}

/**
 * @fn char GetParams*(char*)
 * @brief
 *
 * @param mess
 * @return
 */
char * SetParamsMsg(char *mess)
{
	sprintf(mess, "SCD TESYS-MA %s %s\r\n\r\n"
			      "Terminale: %s\r\n"
			      "Operatore: %s\r\n"
			      "Segnale: %d%s \r\n"
			      "Batteria: %d%s, %0.1fV\r\n"
			      "Temperatura: %0.1f gradi\r\n"
  			      "Soglia: %0.1f gradi\r\n"
				  "Allarme: %s\r\n"
			      "Auto Reset Allarme: %s\r\n"
				  "Num 1: %s\r\n"
				  "Num 2: %s\r\n"
				  "Num 3: %s\r\n\r\n"
				  "Digita #* per il menu comandi.\r\n", App_Version(), App_BuildDate(), gsm.imei, gsm.operator, gsm.signal, "%", gsm.battCharge, "%", gsm.vbatt, GetTemp(), GetTempThreshold(),
				                                        AlarmStatus() == ALARM_ON ? "Abilitato" : "Disabilitato", AlarmAutoEnable() ? "Si" : "No",
						                                gsm.phonebook[0].number, gsm.phonebook[1].number, gsm.phonebook[2].number);
	return mess;
}

/**
 * @fn void DelayAndResetWD(void)
 * @brief attende e reimposta il watchdog
 *
 */
void DelayAndResetWD(void)
{
	HAL_Delay(AT_DELAY);

	WatchdogRefresh();   // se ha ricevuto una risposta reimposta il watchdog
}

/**
 * @fn void SIM800L_SM_Exec(void)
 * @brief
 *
 */
void SIM800L_SM_Exec(void)
{
    static uint32_t time;
    static uint8_t  inactivity_counter = 0;
    // static uint8_t  prompt = 1;
    static char *   psms = sms;

	ATCommand_Reply_TypeDef atreply = ParserInterface()->MsgAnalyze(PARSER_1); // Analize message received on fifo of parser 1 (GSM RX Message Fifo)

	switch (gsm.status)
	{
		case GSM_SEND_AT_COMMAND:

			USART_Write(USART_1, atcommands[gsm.command], 0);

			gsm.status = GSM_WAITING_FOR_REPLY;

			time = HAL_GetTick();

			WatchdogRefresh();   // reimposta il watchdog, se non riceve risposta in tempo utile, intervien il watchdog

		break;

		case GSM_WAITING_FOR_REPLY:

			switch (atreply)
			{
				case ATR_NONE: // no message reply timeout is expired
				break;

				case ATR_LINE: // answer message line

					switch(gsm.command)
					{
						case AT_IMEI:
						{
							char *  mess = GetLineMsg() + 2; // skip first two character
							uint8_t size = strlen(mess) - 4; //

							strncpy(gsm.imei, mess, size);
						}
						break;

						default:
						break;
					}

				break;

				case ATR_COPS: // answer message line
				{
					char *mess = GetLineMsg();

					if (strlen(mess))
					{
						strcpy(gsm.operator, mess);
					}
					else
					{
						HAL_Delay(1000);

						SIM800L_SendATCommand(AT_COPS);
					}
				}
				break;

				case ATR_CSQ: // answer message line
				{
					char *mess = GetLineMsg();

					uint8_t value = atoi(mess);

					switch (value)
					{
						case 0:
							gsm.signal = 25;
						break;

						case 1:
							gsm.signal = 50;
						break;

						default:
							gsm.signal = 75;
						break;

						case 31:
							gsm.signal = 100;
						break;

						case 99:
							gsm.signal = 0;
						break;
					}
				}
				break;

				case ATR_SMS_PROMPT:

					switch (gsm.command)
					{
						case AT_SMS:
						{
							/*if (prompt)
							{
								USART_Write(USART_1, sms, 0); // send the sms

								prompt = 0;
							}*/

							char *line = strstr(psms,"\r\n"); // prende il puntatore alla linea successiva

							if (line) // se c'è una linea successiva
							{
								line[0] = '\0'; // termina la stringa precedente
								line[1] = '\0'; //

								line +=2; // avanza il puntatore alla linea successiva

								USART_Printf(USART_1, "%s\r\n", psms);   // send the current sms line

								psms = line; 				     // imposta il puntatore alla linea successiva
							}
							else
							{
								USART_Write(USART_1, psms, 0);     // send the sms line
							}
						}
						break;

						default:
						break;
					}

					WatchdogRefresh();

				break;

				case ATR_OK:

					switch (gsm.command)
					{
						case AT_ANSWER: // risposta ad una chiamata voce

							gsm.status = GSM_CALL_ANSWERED;

							time = HAL_GetTick();

						break;

						case AT_CALL_PHONEBOOK_ENTRY: // chiama un numero memorizzato nella rubrica
						case AT_CALL_PHONENUMBER:     // chiama un numero

							gsm.status = GSM_CALL_IN_PROGRESS;

							time = HAL_GetTick();

						break;

						case AT_DEL_PHONEBOOK_ENTRY:
						case AT_WRITE_PHONEBOOK_ENTRY:

							SetParamsMsg(gsm.sms[0].mess);

							strcpy(gsm.sms[0].num, SIM800L_GetClipNumber()); // solo il chiamante può aggiungere o eliminare un numero

							SIMM800L_Schedule_SMS(gsm.sms);

						case AT_SMS:

							// prompt = 1;

							psms = sms;

						default:

							gsm.status = GSM_IDLE; // waiting for new command

						break;
					}

					DelayAndResetWD();   // se ha ricevuto una risposta reimposta il watchdog

				break;

				case ATR_ERROR:
				case ATR_NO_CARRIER:
				case ATR_NO_DIALTONE:
				case ATR_NO_ANSWER:
				case ATR_BUSY:

					// prompt = 1;

					gsm.status = GSM_IDLE; // waiting for new command, or manage unsolicted message

					DelayAndResetWD();	   // se ha ricevuto una risposta reimposta il watchdog

				break;

				default:

					// non fa nulla e rimane in attesa il watchdog NON viene reimpostato
					// il watchdog dovrebbe essere reimpostato solo quando viene rilevata dell'attività cioè una risposta ad un comando.

				break;
			}

		break;

		case GSM_CALL_ANSWERED:
		case GSM_CALL_IN_PROGRESS: 	// chiamata in corso

			if (TimSys_TickTimeElapsed(&time, CALL_INACTIVITY_TIMEOUT)) // in generale questo timeout dovrebbe interviene prima del watchdog
			{
				if (++inactivity_counter >=3)
				{
					inactivity_counter = 0;

					SIM800L_SendATCommand(AT_HANG_UP);  // Questa chiamata cambia lo stato della macchina. Al completamento del comando con il ricevimento di 'OK'
														// la macchina va in IDLE se l'OK non viene ricevuto interviene il watchdog dopo 30 sec.
				    break;
				}

				WatchdogRefresh();
			}

			// Controlla la risposta. Il watchdog viene reimpostato solo se viene rilevata una risposta conosciuta.
			// Se la chiamata rimane aperta senza attività nota interviene il watchdog.

			switch (atreply)
			{
				case ATR_RESET:

					SIM800L_HangUp();

					HAL_Delay(2000);

					HAL_NVIC_SystemReset();

				break;

				case ATR_MO_RING:
				case ATR_MO_CONNECTED:

					time = HAL_GetTick();

					inactivity_counter = 0; // importante lasciare: in caso di chamata entrante assicura che il contatore sia azzerato

					WatchdogRefresh();

				break;

				case ATR_NO_CARRIER:        // no carrier Received

					inactivity_counter = 0;

					gsm.status = GSM_IDLE;  // Cambia stato e va in IDLE

					DelayAndResetWD();   	// Chiamata terminata, attende e reimposta il watchdog

				break;

				case ATR_HANGUP: 	        // forza la chiusura della chiamata in corso

					inactivity_counter = 0; // il watchdog interverrà prima del timeout

					DelayAndResetWD();

					SIM800L_HangUp();       // Forza la chiusura della chiamata in modalità immediata, senza cambiare stato, ed attende 'OK' . Se non viene ricevuto interviene il watchdog.

				break;

				case ATR_OK:                // OK Received

					DelayAndResetWD();      // attende e reimposta il watchdog

					time = HAL_GetTick();

					inactivity_counter = 3; // Attività rilevata il timeout interverrà prima del watchdog

					switch (gsm.command)
					{
						case AT_HANG_UP:            // chiamata chiusa attende un certo tempo prima di tornare in idle

							time = HAL_GetTick();   // Non eliminare! Utilizzato dallo stato successivo

							inactivity_counter = 0; // reset timeout counter for use at next in/out call

							gsm.status = GSM_WAITING_FOR_IDLE;

						break;

						case AT_DTMF_SHARP:          // Tono di risposta inviato
						case AT_DTMF_STAR:

							inactivity_counter = 0;  // impedisce che il timeout intervenga prima del watchdog

							SIM800L_HangUp();        // Forza la chiusura della chiamata in modalità immediata, senza cambiare stato, ed attende 'OK' . Se non viene ricevuto interviene il watchdog

						break;

						case AT_READ_PHONEBOOK:      // Lettura rubrica eseguita
						{
							uint8_t idx;

							if (gsm.status == GSM_CALL_ANSWERED)
							{
								idx = 0;

								strcpy(gsm.sms[0].num, SIM800L_GetClipNumber());    // invia il mssaggio al numero entrante
							}
							else
							{
								idx = gsm.call_entry - 1;

								strcpy(gsm.sms[idx].num, gsm.phonebook[idx].number); // invia il messaggio al numero uscente
							}

							SIMM800L_Schedule_SMS_Get_Params(&gsm.sms[idx]);

							gsm.command = AT_DTMF_SHARP;

							USART_Write(USART_1, atcommands[AT_DTMF_SHARP], 0); // Invia il tono di risposta
						}
						break;

						default:
						break;
					}

				break;

				case ATR_GET_PARAMS: // Get ParamS command received

					DelayAndResetWD(); // attende prima di inviare il comando

					ATCommandData_TypeDef data = {.id = AT_CBC, .data = NULL};

					SIM800L_Scheduler_Push(&data); // schedula la lettura dei valori di carica della batteria

					gsm.command = AT_READ_PHONEBOOK;

					USART_Write(USART_1, atcommands[AT_READ_PHONEBOOK], 0); // legge la rubrica in modalità immediata ed attende 'OK'

					time = HAL_GetTick();

					inactivity_counter = 3;	// Il timeout interverrà prima del watchdog

				break;

				case ATR_DTMF_STAR: // Star Tone sending command received

					DelayAndResetWD();

					gsm.command = AT_DTMF_STAR;

					USART_Write(USART_1, atcommands[AT_DTMF_STAR], 0);

					time = HAL_GetTick();

					inactivity_counter = 3; // Il timeout interverrà prima del watchdog

				break;

				case ATR_DTMF_SHARP: // Sharp Tone sending command received

					DelayAndResetWD();

					gsm.command = AT_DTMF_SHARP;

					USART_Write(USART_1, atcommands[AT_DTMF_SHARP], 0);

					time = HAL_GetTick();

					inactivity_counter = 3;

				break;

				case ATR_HELP: // Help command received
				{
					DelayAndResetWD();

					uint8_t idx;

					if (gsm.status == GSM_CALL_ANSWERED)
					{
						idx = 0;

						strcpy(gsm.sms[0].num, SIM800L_GetClipNumber());    // invia il mssaggio al numero entrante
					}
					else
					{
						idx = gsm.call_entry - 1;

						strcpy(gsm.sms[idx].num, gsm.phonebook[idx].number); // invia il messaggio al numero uscente
					}

					sprintf(gsm.sms[idx].mess,"\r\nSCD TESYS-MA %s %s\r\n\r\n"
												    "#*  Aiuto\r\n"          							// 10
												    "### Alarme OFF\r\n"    							// 16
												    "##* Alarme ON\r\n"    								// 15
												    "##0 Auto Reset OFF\r\n"
													"##1 Auto Reset ON\r\n"
												    "*** Parametri\r\n"   								// 15
												    "**GGD** Imposta Temperat. GG:[00-99] D:[0-9]\r\n"  // 46
												    "#X## Elimina numero X:[1-3]\r\n"                   // 29
												    "*X*Num** Modifica numero X:[1-3]\r\n", App_Version(), App_BuildDate());            // 34

					SIMM800L_Schedule_SMS(&gsm.sms[idx]); // schedula l'invio degli SMS

					gsm.command = AT_DTMF_SHARP;

					USART_Write(USART_1, atcommands[AT_DTMF_SHARP], 0);  // invia il tono di risposta in modalità immediata, ed attende OK. Se non viene ricevuto interviene il watchdog

					time = HAL_GetTick();

					inactivity_counter = 3;
				}
				break;

				default:

					// do nothing: watchdog not refreshed

				break;
			}

		break; // GSM_CALL_ANSWERED

		case GSM_IDLE: // waiting for new command, or manage unsolicted message

			if (gsm.fifo_items) // check scheduler
			{
				ATCommandData_TypeDef sch_command;

				SIM800L_Scheduler_Pop(&sch_command);

				switch (sch_command.id) // get at command from scheduler
				{
					case AT_WRITE_PHONEBOOK_ENTRY:

						SIMM800L_AddPhonebookEntry((PhonebookEntry_TypeDef *) sch_command.data);

					break;

					case AT_DEL_PHONEBOOK_ENTRY:

						SIMM800L_DelPhonebookEntry( *((PhonebookIdEntry_TypeDef *)sch_command.data) );

					break;

					case AT_SMS: // send generic scheduled SMS

						// prompt = 1;

						psms = sms;

						SIMM800L_SMS((SMS_TypeDef *) sch_command.data);

					break;

					case AT_SMS_GET_PARAMS: // send the scheduled params SMS

						// prompt = 1;

						SetParamsMsg(((SMS_TypeDef *) sch_command.data)->mess);

						SIMM800L_SMS((SMS_TypeDef *) sch_command.data);

					break;

					case AT_CALL_PHONEBOOK_ENTRY: // Call a phonebook entry

						SIMM800L_CallPhonebookEntry(*((uint8_t *)sch_command.data));

					break;

					case AT_WELCOME_SMS:

						strcpy(gsm.sms[0].num, gsm.phonebook[0].number);

						SIMM800L_Schedule_SMS_Get_Params(gsm.sms);

					break;

					case AT_SMS_DEL_ENTRY:
					break;

					default:

						if (sch_command.data == NULL) // simple command
						{
							SIM800L_SendATCommand(sch_command.id);
						}

					break;
				}
			}
			else // check unsolicited
			{
				// Attenzione quando è in IDLE inviare periodicamente il comando AT per controllare la connessione dati seriale col modulo.
				// in questo modo se il comando resta appeso, interverrà il watchdog

				switch (atreply)
				{
					case ATR_RING: 		  // chiamata in ingresso

						if (gsm.ring==0)
						{
							memset(gsm.clipnumber,0,sizeof(gsm.clipnumber)); // clear clip number
						}

						if (++gsm.ring >1)
						{
							gsm.ring = 0;

							SIM800L_Answer(); // risponde

							time = HAL_GetTick();
						}

					break;

					default:

						if (TimSys_TickTimeElapsed(&time, CALL_INACTIVITY_TIMEOUT))
						{
							SIM800L_SendATCommand(AT_AT);

							time = HAL_GetTick();
						}
						else
						{
							WatchdogRefresh();
						}

					break;
				}
			}

		break;

		case GSM_ERROR:

		break;

		case GSM_WAITING_FOR_IDLE:

			if (TimSys_TickTimeElapsed(&time, HANGUP_TIMEOUT))
			{
				gsm.status = GSM_IDLE; // return to idle after HANGUP
			}

		break;

		case GSM_WAITING_FOR_READY:
		{
			static uint8_t  waiting = 0;
			static uint32_t time    = 0;

			switch (atreply)
			{
				case ATR_READY:

					gsm.status = GSM_IDLE; // go to to idle. Se non va in idle entro il timeout del watchdog, intervinviene il watchdog e riavvia.

					WatchdogRefresh();

					HAL_Delay(GSM_STARTING_DELAY);

				break;

				case ATR_OK:

					if (waiting < 2)
					{
						waiting = 2; // OK found
					}

				break;

				default:

					 if (waiting == 0)
					 {
						 USART_Write(USART_1, atcommands[AT_AT], 0);

						 time = HAL_GetTick() + 1000;

						 waiting = 1;
					 }
					 else
					 if (HAL_GetTick() > time && waiting == 1)
					 {
						 waiting = 0; // continua ad inviare AT finchè non vi è risposta. Il watchdog dovrebbe intervenire entro 30 sec.) per riavviare
					 }

				break;
			}
		}
		break;

		default:
		break;
	}
}

/**
 * @fn void print_status(void)
 * @brief
 *
 */
void print_status(void)
{
	static uint32_t time1 = 0;

	if (TimSys_TickTimeElapsed(&time1, 5000))
	{
		char mess[64];

		char *status[8] = {

			[GSM_WAITING_FOR_READY] = "WAITING FOR READY",
			[GSM_WAITING_FOR_IDLE]  = "WAITING FOR IDLE",
			[GSM_IDLE]              = "IDLE",
			[GSM_SEND_AT_COMMAND]   = "SEND AT COMMAND",
			[GSM_WAITING_FOR_REPLY] = "WAITING FOR AT REPLY",
			[GSM_CALL_IN_PROGRESS]  = "CALL_IN_PROGRESS",
			[GSM_CALL_ANSWERED]     = "CALL_ANSWERED",
			[GSM_ERROR]             = "ERROR" ,
		};

		sprintf(mess,"\r\nGSM Status: %s\r\n",status[GSM_Status()]);

		USART_Write(USART_2, mess, 0);
	}
}

