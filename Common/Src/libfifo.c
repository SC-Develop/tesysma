/**
 * @file  libfifo.c  - https://github.com/SC-Develop/tesysma
 *
 * @brief circular queue management module, suitable also for serial character device
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "libfifo.h"
#include "memory.h"

/**
 * @fn void fifo_init(fifo_TypeDef*, char*)
 * @brief
 *
 * @param fifo
 * @param buffer
 */
void ch_fifo_init(Fifo_TypeDef *fifo, char *buffer, uint16_t buffersize)
{
  	fifo->first  = 0;
  	fifo->next   = 0;
  	fifo->items  = 0;
  	fifo->size   = buffersize;
  	fifo->buffer = buffer;

  	memset(buffer,0,buffersize);
}

/**
 * @fn uint8_t ch_fifo_push(fifo_TypeDef, char)
 * @brief
 *
 * @param fifo
 * @param ch
 * @return
 */
uint8_t ch_fifo_push(Fifo_TypeDef *fifo, char ch)
{
  	if (fifo->items == fifo->size)
	{
	   return 0; // coda piena
	}

	fifo->buffer[fifo->next] = ch;

	if (++fifo->next == fifo->size)
	{
	   fifo->next = 0; // circular fifo
	}

	fifo->items++;

	return 1;
}

/**
 * @fn uint8_t ch_fifo_push(fifo_TypeDef, char)
 * @brief
 *
 * @param fifo
 * @param ch
 * @return
 */
uint8_t ch_fifo_pop(Fifo_TypeDef *fifo, char *ch)
{
   if (fifo->items==0)
   {
      return 0; // null char = empty fifo
   }

   *ch = fifo->buffer[fifo->first];

   if (++fifo->first==fifo->size)
   {
	   fifo->first=0;
   }

   fifo->items--;

   return 1;
}

/**
 * @fn uint8_t ch_fifo_pop(Fifo_TypeDef*, char*)
 * @brief
 *
 * @param fifo
 * @param ch
 * @return
 */
uint8_t ch_fifo_get(Fifo_TypeDef *fifo, char *ch)
{
   if (fifo->items==0)
   {
      return 0; // null char = empty fifo
   }

   *ch = fifo->buffer[fifo->first];

   return 1;
}

