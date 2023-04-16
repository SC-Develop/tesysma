/*
 * parser.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef PARSER_H_
#define PARSER_H_

    #include "main.h"
    #include "libparser.h"

	typedef enum {
		ATR_NONE = PR_MAX,  // do not overlap PR_XXX value
		ATR_OK,
		ATR_ERROR,
		ATR_NO_CARRIER,
		ATR_NO_DIALTONE,
		ATR_NO_ANSWER,
		ATR_BUSY,
		ATR_HANGUP,
		ATR_SHARP,
		ATR_CME_ERROR,
		ATR_RING,
		ATR_CLIP,
		ATR_CPBR,
		ATR_SMS_PROMPT,
		ATR_DTMF_SHARP,
		ATR_DTMF_STAR,
		ATR_GET_PARAMS,
		ATR_HELP,
		ATR_RESET,
		ATR_CBC,
		ATR_READY,
		ATR_MO_CONNECTED,
		ATR_MO_RING,
		ATR_LINE,
		ATR_COPS,
		ATR_CSQ,
		ATR_NULL,
	} ATCommand_Reply_TypeDef;

    uint8_t IsHexDigit(char digit);
    uint8_t ParserInit(void);
    char *GetLineMsg(void);


#endif /* PARSER_H_ */
