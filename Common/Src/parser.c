/**
 * @file   parser.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libparser.h>
#include "parser.h"
#include "stm32_lib_usart.h"
#include "SIM800L.h"
#include "sm_alarm.h"

typedef enum {
	MID_OK = 0,
	MID_BUSY,
	MID_NOCARRIER,
	MID_NODIALTONE,
	MID_NOANSWER,
	MID_CME_ERROR,
	MID_ERROR,
	MID_RING,
	MID_DTMF,
	MID_CLIP,
	MID_CPBR,
	MID_PROMPT,
	MID_CBC,
	MID_READY,
	MID_MO_RING,
	MID_MO_CONNECTED,
	MID_LINE, // generic line string
	MID_COPS,
	MID_CSQ,
	MID_NULL,
	MID_MAX
} MessageId_TypeDef;

// - Local Function Prototypes ------------------------------------------------------------------------------- /

static uint8_t gsm(char *arg, uint8_t cmd_id);
static void ClearMessage(void);

// - Local variables ----------------------------------------------------------------------------------------- /

static Commands_TypeDef gsm_reply[MID_MAX] = {
	[MID_OK] 		   = {"OK\r\n"         , NULL  , gsm, NULL,},
	[MID_BUSY] 		   = {"BUSY\r\n"       , NULL  , gsm, NULL,},
	[MID_NOCARRIER]    = {"NO CARRIER\r\n" , NULL  , gsm, NULL,},
	[MID_NODIALTONE]   = {"NO DIALTONE\r\n", NULL  , gsm, NULL,},
	[MID_NOANSWER] 	   = {"NO ANSWER\r\n"  , NULL  , gsm, NULL,},
	[MID_CME_ERROR]    = {"+CME ERROR:"	   , NULL  , gsm, NULL,},
	[MID_ERROR]		   = {"ERROR\r\n"      , NULL  , gsm, NULL,},
	[MID_RING]		   = {"\r\nRING\r\n"   , NULL  , gsm, NULL,},
	[MID_DTMF]   	   = {"+DTMF: "        , "\r\n", gsm, NULL,},
	[MID_CLIP]   	   = {"+CLIP: "        , "\r\n", gsm, NULL,},
	[MID_CPBR]   	   = {"+CPBR: "        , "\r\n", gsm, NULL,},
	[MID_PROMPT]   	   = {">"              , NULL  , gsm, NULL,},
	[MID_CBC]   	   = {"+CBC: "         , "\r\n", gsm, NULL,},
	[MID_READY]        = {"SMS READY"      , "\r\n", gsm, NULL,},
	[MID_MO_RING]      = {"\r\nMO RING"    , "\r\n", gsm, NULL,},
	[MID_MO_CONNECTED] = {"MO CONNECTED"   , "\r\n", gsm, NULL,},
	[MID_LINE]         = {"\r\n"           , "\r\n\r\n", gsm, NULL,}, // unrecognized generic answer line string (could be a generic command answer)
	[MID_COPS]         = {"+COPS: "        , "\r\n", gsm, NULL,},     // unrecognized generic answer line string (could be a generic command answer)
	[MID_CSQ]          = {"+CSQ: "         , "\r\n", gsm, NULL,},     // unrecognized generic answer line string (could be a generic command answer)
	[MID_NULL] 	       = {NULL             , NULL  , NULL, NULL,},	  // USART2 port commands array initialization
};

static char * ser_prefix = "#";

static char mess[1024] = "\0"; // stringa nulla: contiene solo il carattere terminatore

static char line_msg[256] = "\0";

// - Exported Functions -------------------------------------------------------------------------------------- /

/**
 * @brief
 */
uint8_t IsHexDigit(char digit)
{
	return (isdigit(digit) || (digit >='A' && digit <= 'F'));
}

/**
 * @fn uint8_t IsDigit(char)
 * @brief
 *
 * @param digit
 * @return
 */
uint8_t IsDigit(char digit)
{
	return (digit >='0' && digit <= '9');
}

/**
 * @fn uint8_t CharDigitToInt(char)
 * @brief
 *
 * @param digit
 * @return
 */
uint8_t CharDigitToInt(char digit)
{
	return digit - 48;
}

/**
 * @fn char ExtractNumber*(char*)
 * @brief
 *
 * @param
 * @return
 */
char *FindQuotedString(char *mess)
{
	char *string = strstr(mess,"\""); // cerca il primo doppio apice nella stringa

	uint8_t len = 0;

	if (string)
	{
		string++;

		while (string[len] != '"') // finquando non trova il secondo doppio apice
		{
			len++;

			if (string[len]=='\r') // se si arriva a fine stringa senza aver trovato il doppio apice
			{
				return NULL;  // il formato della stringa non è valido
			}
		}
	}

	string[len] = '\0';

	return string;
}

/**
 * @brief inits the serial devices command parsers: USART2 and USB
 */
uint8_t ParserInit(void)
{
	ParserInterface()->Init(PARSER_1, USART_RxFifo(USART_1), ser_prefix, gsm_reply);

	return 1;
}

/**
 * @fn char GetLineMsg*(void)
 * @brief
 *
 * @return
 */
char *GetLineMsg(void)
{
	return line_msg;
}

// - Local Functions ----------------------------------------------------------------------------------------- /

/**
 * @fn void ClearMessage(void)
 * @brief
 *
 */
static void ClearMessage(void)
{
	uint16_t size = sizeof(mess);

	memset(mess,0, size);
}

/**
 * @fn void ClearMessage(void)
 * @brief
 *
 */
static void ClearLineMsg(void)
{
	uint16_t size = sizeof(line_msg);

	memset(line_msg,0, size);
}


/**
 * @fn uint8_t gsm(char*)
 * @brief
 *
 * @param arg
 * @return
 */
static uint8_t gsm(char *arg, uint8_t cmd_index)
{
	static uint8_t len = 0;

	switch (cmd_index)
    {
		case MID_COPS:
		{
		   char * line = FindQuotedString(arg);

		   ClearLineMsg();

		   strcpy(line_msg, line);

           return ATR_COPS;
		}
		break;

		case MID_CSQ:
		{
		   ClearLineMsg();

		   char *  signal = arg + strlen(gsm_reply[MID_CSQ].prefix);
		   uint8_t size   = (uint32_t) strstr(arg,",") - (uint32_t) signal;

		   strncpy(line_msg, signal, size);

		   return ATR_CSQ;
		}
		break;

		case MID_LINE:

			ClearLineMsg();

			strcpy(line_msg,arg);

			return ATR_LINE;

		break;

    	case MID_OK:

    		// USART_Write(USART_2, "\r\nComando eseguito!\r\n", 0);

    		return ATR_OK;

    	break;

    	case MID_BUSY:

    		// USART_Write(USART_2, "\r\nOccupato!\r\n", 0);

			return ATR_BUSY;

    	break;

    	case MID_NOANSWER:

    		// USART_Write(USART_2, "\r\nNessuna risposta!\r\n", 0);

    		return ATR_NO_ANSWER;

    	break;

    	case MID_NOCARRIER:

    		// USART_Write(USART_2, "\r\nChiamata terminata!\r\n", 0);

    		len = 0;

    		ClearMessage();

    		return ATR_NO_CARRIER;

    	break;

    	case MID_NODIALTONE:

    		// USART_Write(USART_2, "\r\nTono di chiamata assente!\r\n", 0);

    		return ATR_NO_DIALTONE;

    	break;

    	case MID_ERROR:

    		// USART_Write(USART_2, "\r\nErrore comando!\r\n", 0);

    		len = 0;

    		ClearMessage();

    		return ATR_ERROR;

    	break;

    	case MID_CBC:
    	{
    		uint8_t charge;
    		float   vbat;

    		char *tok = strtok(strstr(arg, ","),",");

    		charge = atoi(tok);

    		tok = strtok(NULL,",");

    		vbat = atof(tok)/1000.0;

    		SetBattCharge(charge);

    		SetVBatt(vbat);

    		return ATR_CBC;
    	}
    	break;

    	case MID_RING:

    		if (strstr(arg,gsm_reply[cmd_index].prefix))
    		{
    			len = 0;

				ClearMessage();

    			return ATR_RING;
    		}

    	break;

    	case MID_CLIP:
    	{
			char *number = strstr(arg,"\"");

			uint8_t len = 0;

			number++;

			while (number[len] != '"')
			{
				len++;

				if (number[len]=='\r')
				{
					return ATR_NONE;
				}
			}

			number[len] = '\0';

			SIM800L_SetClipNumber(number);

			return ATR_CLIP;
    	}
    	break;

    	case MID_MO_RING:

    		return ATR_MO_RING;

    	break;

    	case MID_MO_CONNECTED:

    		return ATR_MO_CONNECTED;

    	break;

    	case MID_CPBR:
    	{
    		char *p = strstr(arg," ") + 1;

    		uint8_t entry = CharDigitToInt(*p); // phonebook entry

    		char *number = FindQuotedString(p);

    		SetPhonebookEntry(number, entry);

    		return ATR_CPBR;
    	}
    	break;

    	case MID_DTMF:

    		mess[len++] = arg[7]; // build message string

    		switch (mess[0])
    		{
    			case '*': // il messaggio inizia con un asterisco
    			{
    				if (mess[1]=='*') 		// ** due star inziali
    				{
    					if (len>=7)         // messaggio di lunghezza opportuna
    					{
    						if (strstr(mess + 5, "**")) // **xxx** se il messaggio termina con due asterischi
    						{
    							if (IsDigit(mess[2]) && IsDigit(mess[3]) && IsDigit(mess[4]))
    							{
    								uint8_t idx10 = CharDigitToInt(mess[2]) * 10;
    								uint8_t idx1  = CharDigitToInt(mess[3]);
    								float   ddx1  = CharDigitToInt(mess[4])/10.0;

    								float   temp = idx10 + idx1 + ddx1;

    								SetTempThreshold(temp);

    								len = 0;

		    						ClearMessage();

    								return ATR_GET_PARAMS;
    							}
    							else
    							{
    								len = 0;

    								ClearMessage();

    								return ATR_HANGUP;
    							}
    						}
    						else // errore formato
    						{
    							len = 0;

    							ClearMessage();

    							return ATR_HANGUP;
    						}
    					}

    					if (mess[2]=='*') 	// *** => ottiene i parametri correnti
    					{
    						len = 0;

    						ClearMessage();

    						return ATR_GET_PARAMS;
    					}
    				}

    				char *msg_end = strstr(mess,"**");

    				if (mess[2]=='*' && msg_end) // aggiunge un numero di telefono
					{
						*msg_end = '\0'; // terminate the string

						uint32_t len = strlen(mess + 3);

						if (mess[1]>='0' && mess[1] <'4' && len <= MAX_NUM_LENGTH)
						{
							uint8_t phonebookEntry = CharDigitToInt(mess[1]); //mess[1]-48; // normalize the ascii code ('1' is equivalent to 1 decimal)

							SIMM800L_Schedule_AddPhonebookEntry(mess+3, phonebookEntry);

							len = 0;

							ClearMessage();

							return ATR_DTMF_SHARP; // return tone confirm
						}
						else
						{
							len = 0;

							ClearMessage();

							return ATR_DTMF_STAR; // Return tone error
						}
					}
    			}
    			break;

    			case '#':

    				if (len < 3)
					{
    				   if (mess[1] == '*')
    				   {
    					   return ATR_HELP;
    				   }

				       break;
					}

    				switch (mess[1])
    				{
       					case '#':

    						switch (mess[2])
    						{
    							case '#': // set alam to off

    								len = 0;

    								ClearMessage();

    								EnableAlarm(ALARM_OFF);

    								return ATR_GET_PARAMS;

    							break;

    							case '*': // set alarm to on

    								len = 0;

    								ClearMessage();

    								EnableAlarm(ALARM_ON);

    								return ATR_GET_PARAMS;

								break;

    							case '0': // ##0
    							case '1': // ##1

    								SetAlarmAutoEnable(CharDigitToInt(mess[2])); // disable alarm auto reset

    								return ATR_GET_PARAMS;

    							break;

    							default:
    								return ATR_HANGUP;
    							break;
							}

						break;

						case '0':

							if (mess[2]=='#') // #0# RESET !!!
							{
								return ATR_RESET;
							}

						break;

    					default:

    						if (mess[1] > 47 && mess[1] < 58) // Cifre ASCII da '0' => '9'
    						{
								if (strstr((mess + 2), "##")) // Elimina un numero dalla rubrica
								{
									if (SIMM800L_Schedule_DelPhonebookEntry(CharDigitToInt(mess[1])))
									{
										len = 0;

										ClearMessage();

										return ATR_DTMF_SHARP;
									}
									else
									{
										len = 0;

										ClearMessage();

										return ATR_DTMF_STAR; // Return tone error
									}
								}
    						}
    						else
    						{
    							len = 0;

								ClearMessage();

								return ATR_DTMF_STAR; // Return tone error
    						}

    					break;

    				} // end switch

    				if (len > 3)
    				{
    					return ATR_HANGUP; // Return tone error
    				}

    			break; // end case #

				default:
					len = 0;

					ClearMessage();

					return ATR_HANGUP; // Return tone error
				break;

    		} // end switch

    	break;

    	case MID_PROMPT:

    		return ATR_SMS_PROMPT;

    	break;

    	case MID_READY:

    		return ATR_READY;

    	break;
    }

	return ATR_NONE;
}

