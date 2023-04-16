/*
 * libparser.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 *  @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef _LIB_PARSER_H_
#define _LIB_PARSER_H_

	#include "main.h"
	#include "libparser.def.h"
	#include "libfifo.h"

    #define BUFF_LEN 	(BUF_LEN < CMD_LEN ? CMD_LEN) : BUF_LEN
	#define NULLCH		0x00
	#define BACKSPACE 	0x08
	#define	LF			0x0A
	#define	_CR			0x0D
	#define	TAB			0x11
	#define ESCAPE      0x1B
	#define	SPACE		0x20
	#define DEL			0x7F

    typedef enum
    {
      PR_PROMPT,     // PRINT PROMP REQUIRED
	  PR_OK,         // PRINT OK    REQUIRED
	  PR_ERROR,      // PINRT ERROR REQUIRED
	  PR_BACKSPACE,  // PRINT BACKSPACE REQUIRED
	  PR_ECHO,       // PRINT ECHO LAST CHAR REQUIRED
	  PR_IGNORED,
	  PR_OVERFLOW,
	  PR_HEX_FORMAT_ERROR,
	  PR_CAN_MESSAGE_FORMAT_ERROR,
	  PR_MAX,
    } ParserResult_TypeDef;

    typedef uint8_t (*ExecCallback)(char *cmd, uint8_t cmd_index);
    typedef uint8_t (*EchoCallback)(ParserResult_TypeDef result, char ch);

    typedef struct
    {
    	char * prefix;
    	char * suffix;
    	ExecCallback execCallback;
    	EchoCallback echoCallback;
    } Commands_TypeDef;

    /*typedef struct
	{
		char * prefix;
		char * suffix;
		ExecCallback execCallback;
		EchoCallback echoCallback;
	} Messages_TypeDef;
    */

    typedef struct
    {
    	uint8_t (*Init)      (ParserId_TypeDef id, Fifo_TypeDef *rxFifo, char *prefix, Commands_TypeDef *c);
    	uint8_t (*Clear)     (ParserId_TypeDef id);
    	uint8_t (*CmdAnalyze)(ParserId_TypeDef id);
    	uint8_t (*MsgAnalyze)(ParserId_TypeDef id);
    } ParserInterface_TypeDef;

    ParserInterface_TypeDef *ParserInterface(void);

#endif /* _LIB_PARSER_H_ */

