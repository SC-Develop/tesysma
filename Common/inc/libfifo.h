/*
 * libfifo.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 *  @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef SRC_LIBFIFO_H_
#define SRC_LIBFIFO_H_

#include"main.h"

typedef struct {
  uint16_t first; // order number of first in fifo item (head of fifo=nect item to pop)
  uint16_t next;  // order number of next item to push
  uint16_t items; // number of items in fifo
  uint16_t size;
  char *buffer;
} Fifo_TypeDef;

void    ch_fifo_init(Fifo_TypeDef *fifo, char *buffer, uint16_t buffersize);
uint8_t ch_fifo_push(Fifo_TypeDef *fifo, char ch);
uint8_t ch_fifo_pop(Fifo_TypeDef *fifo, char *ch);
uint8_t ch_fifo_get(Fifo_TypeDef *fifo, char *ch);

#endif /* SRC_LIBFIFO_H_ */
