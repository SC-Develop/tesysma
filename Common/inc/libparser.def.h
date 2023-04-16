/*
 * libparser.def.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 *  @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef INC_LIBPARSER_DEF_H_
#define INC_LIBPARSER_DEF_H_

typedef enum {
  PARSER_1 = 0,
  PARSER_2 = 1,
  PARSER_3 = 2,
  PARSER_4 = 3,
} ParserId_TypeDef;

#define MAXPARSER 1

#define CMD_LEN  128
#define BUF_LEN  128

#endif /* INC_LIBPARSER_DEF_H_ */
