/**
 * @file libparser.c - https://github.com/SC-Develop/tesysma
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
#include <libfifo.h>

/**
 * Defines -------------------------------------------------------------------------------------------------- /
 */

#define PARSER_NUM     ((MAXPARSER == 0) ? 1 : MAXPARSER)  	// do not change
#define PARSER_ID_MAX  PARSER_NUM-1 						// do not change

/**
 * Type definitions ----------------------------------------------------------------------------------------- /
 */

typedef struct
{
	uint8_t i;
	uint8_t echo;
	uint8_t rx_len;   			// numero caratteri nel buffer
	uint8_t rx_idx;  			// indice prossimo carattere da inserire nel buffer
	uint8_t cmd_len;        	// max lenght of command buffer

	char *prefix;   			// interface prefix, must be a null terminated string
	char cmd[CMD_LEN];          // internal command/messages string buffer

	Fifo_TypeDef     *rxFifo;   // received data buffer fifo
	Commands_TypeDef *commands; // array of command/messages  callback
} Parser_TypeDef;

/**
 * Local Function Prototypes -------------------------------------------------------------------------------- /
 */

static uint8_t command_analyze(ParserId_TypeDef id);
static uint8_t message_analyze(ParserId_TypeDef id);
static uint8_t parser_init(ParserId_TypeDef id, Fifo_TypeDef *rxFifo, char *prefix, Commands_TypeDef *c);
static uint8_t parser_clear(ParserId_TypeDef id);

static uint8_t message_parsing(Parser_TypeDef *parser, char ch);
static ParserResult_TypeDef command_parsing(Parser_TypeDef *parser, char ch);

/**
 * Local variables ------------------------------------------------------------------------------------------ /
 */

static Parser_TypeDef parser[PARSER_NUM];

static ParserInterface_TypeDef Interface = {
	.Init    	= parser_init,
	.CmdAnalyze = command_analyze,
	.MsgAnalyze = message_analyze,
	.Clear   	= parser_clear,
};

/**
 * Exported Functions --------------------------------------------------------------------------------------- /
 */

/**
 * @fn ParserInterface_TypeDef ParserInterface*(void)
 * @brief
 *
 * @return
 */
ParserInterface_TypeDef *ParserInterface(void)
{
	return &Interface;
}

/**
 * @fn uint8_t clear_parser(Parser_TypeDef*)
 * @brief
 *
 * @param p
 * @return
 */
static uint8_t clear_parser(Parser_TypeDef *p)
{
	p->i      	  = 0;
	p->rx_idx     = 0;
	p->rx_len     = 0;

	memset(p->cmd,0,CMD_LEN);

	return 1;
}

/**
 * @fn uint8_t is_valid_id(ParserId_TypeDef)
 * @brief
 *
 * @param id
 * @return
 */
static uint8_t is_valid_id(ParserId_TypeDef id)
{
	return ( id <= PARSER_ID_MAX );
}

/**
 * @fn uint8_t parser_clear(ParserId_TypeDef)
 * @brief
 *
 * @param id
 * @return
 */
static uint8_t parser_clear(ParserId_TypeDef id)
{
	if (!is_valid_id(id))
	{
	   return 0;
	}

	clear_parser(parser + id);

	return 1;
}

/**
 * @brief  initialize the parser p.
 * @param  p      the parser you want to initialize
 * @prefix the prefix of parser command: the command must begin with 'prefix'
 * @parser command list: must end with NULL pointer
 */
static uint8_t parser_init(ParserId_TypeDef id, Fifo_TypeDef *rxFifo, char *prefix, Commands_TypeDef *c)
{
	if (!is_valid_id(id))
	{
	   return 0;
	}

	Parser_TypeDef *p = (parser + id);

	p->prefix     = prefix;
	p->commands	  = c;
	p->echo       = 1;
	p->rxFifo     = rxFifo;

	parser_clear(id);

	return 1;
}

/**
 * Local Functions ------------------------------------------------------------------------------------------ /
 */

/**
 * @fn ParserResult_TypeDef ch_parsing(Parser_TypeDef*, char)
 * @brief parse the last character received and call the callback functions if commad is recognized.
 *
 * @param p
 * @param ch
 * @return
 */
static ParserResult_TypeDef command_parsing(Parser_TypeDef *p, char ch)
{
	ch = toupper(ch);                       // uppercase character

	uint8_t prefix_len = strlen(p->prefix);

	if (p->i < prefix_len)     				// check the prefix_len character: only prefix_len characters are allowed
	{
		if (ch == _CR)
		{
		   if (p->i==0)      			   	// if the first character of command is CR return the prompt
		   {
			  return PR_PROMPT;				// print the prompt and do nothing
		   }
		   else
		   {
			  p->i = 0;

			  return PR_ERROR;
		   }
		}

		if (ch == p->prefix[p->i]) 		// accept n-th char of parser prefix
		{
			p->cmd[p->i++] = ch;   		// get the n-th char of prefix

			return PR_ECHO; 			// character accepted
		}

		return PR_IGNORED;  			// do nothing: character refused
	}

	if (p->i >= prefix_len)
    {
		switch (ch)
		{
			case BACKSPACE:

			  if (p->rx_len>0) // override the previous character
			  {
				p->rx_len--;
				p->rx_idx--;
			  }

            break;

			case DEL:
			case TAB:
		  	  return PR_IGNORED;
		  	break;

			case _CR:
			case  LF: 	// return key pressed: end of command detected
			{
				p->cmd[p->i] = NULLCH;      	      // terminate the string command (without CR/LF)

				p->i = 0;

				char *cmd  = p->cmd + prefix_len;     // skip parser prefix

				Commands_TypeDef *cmds = p->commands; // get commands list

				for (uint8_t n=0; cmds->prefix; n++, cmds++)          // itera i comandi fin quando non si trova un command prefix nullo (ultima linea dell'array dei comandi è nulla)
				{
					prefix_len = strlen(cmds->prefix);

					if (strncasecmp(cmd, cmds->prefix, prefix_len)==0) // compare command with current command prefix: check if command starts with command prefix
					{
						cmd += prefix_len; // skip to command args

						if (strlen(cmd))
						{
							if (cmds->execCallback)
							{
								return cmds->execCallback(cmd,n);
							}
						}
					}
				}

				return PR_ERROR;
			}

			break;
		} // end switch
	}

    if (p->i >= CMD_LEN - 1) // if there's no more space in the buffer do nothing
    {
    	p->i      = 0;
    	p->cmd[0] = NULLCH;

		return PR_OVERFLOW;
    }

	p->cmd[p->i++] = ch; // get current char

	return PR_ECHO;
}

/**
 * @fn ParserResult_TypeDef message_parsing(Parser_TypeDef*, char)
 * @brief add the character ch to parser message buffer, and parse the message
 *
 * @param p
 * @param ch
 * @return
 */
static uint8_t message_parsing(Parser_TypeDef *p, char ch)
{
	ch = toupper(ch);    	 				// uppercase character

	p->cmd[p->i++] = ch;     				// add current char to string to parse

	char *rcv_msg = p->cmd;  				// get received message string pointer

	Commands_TypeDef *msgs = p->commands; 	// get message list

	for (uint8_t n=0; msgs->prefix; n++, msgs++)  		// itera i comandi fin quando non si trova un command prefix nullo (ultima linea dell'array dei comandi è nulla)
	{
		char *message = strstr(rcv_msg, msgs->prefix);  // estrae il puntatore al prefix dal messaggio ricevuto

		if (message)   								    // compare message with current message prefix: check if command starts with command prefix
		{
			// La lunghezza del messaggio ricevuto deve essere pari o maggiore alla somma delle lunghezze del prefisso e del suffisso (lunghezza completa del messaggio da analizzare)

			uint8_t message_len = strlen(message);      // lunghezza del messaggio ricevuto
			uint8_t prefix_len  = strlen(msgs->prefix); // lunghezza del prefisso del messaggio
			uint8_t suffix_len  = strlen(msgs->suffix); // lunghezza del suffisso del messaggio

			if (message_len >= (prefix_len + suffix_len) )  // se il messaggio ricevuto ha la lunghezza minima richiesta
			{
				if (msgs->suffix) // controlla il suffisso
				{
					if (!strstr(message + prefix_len, msgs->suffix))  // check the message suffix at the end of message prefix
					{
						continue; // suffix not found
					}
				}

				if (msgs->execCallback)
				{
					uint8_t result;

					result = msgs->execCallback(message, n);

					p->i = 0;

					memset(p->cmd, NULLCH, CMD_LEN);

					return result;
				}
			}
		}
	}

    if (p->i >= CMD_LEN - 2) // if there's no more space in the buffer do nothing, leave last char for string terminating null char
    {
    	p->i = 0;

    	memset(p->cmd,NULLCH,CMD_LEN);

		return PR_OVERFLOW;
    }

	return PR_ECHO;
}

/**
 * @fn uint8_t analyze(ParserId_TypeDef)
 * @brief Analize the last characters received
 *
 * @pre
 * @post
 * @param id
 * @return
 */
static uint8_t message_analyze(ParserId_TypeDef id)
{
	if (!is_valid_id(id))
	{
		return 0;
	}

	char ch;

	Parser_TypeDef *p = (parser + id); 						// get the parser

	while (ch_fifo_pop(p->rxFifo, &ch)!=NULLCH)
	{
	   uint8_t result = message_parsing(p,ch); 				// parse the current char and execute command if recognized

	   if (p->echo)
	   {
		  if (p->commands->echoCallback) 					// se la callback è impostata
		  {
			  p->commands->echoCallback(result, ch);		// character echo managment
		  }
	   }

	   if (result != PR_ECHO && result != PR_OVERFLOW)
	   {
		   return result;
	   }
	}

	return 0;
}

/**
 * @fn uint8_t analyze(ParserId_TypeDef)
 * @brief Analize the last characters received
 *
 * @pre
 * @post
 * @param id
 * @return
 */
static uint8_t command_analyze(ParserId_TypeDef id)
{
	if (!is_valid_id(id))
	{
		return 0;
	}

	char ch;

	Parser_TypeDef *p = (parser + id); // get the parser

	while (ch_fifo_pop(p->rxFifo, &ch)!=NULLCH)
	{
	   ParserResult_TypeDef result = command_parsing(p,ch);  // parse the current char and execute command if recognized

	   if (p->echo)
	   {
		  if (p->commands->echoCallback) 					// se la callback è impostata
		  {
			  p->commands->echoCallback(result, ch);		// character echo managment
		  }
	   }
	}

	return 1;
}

