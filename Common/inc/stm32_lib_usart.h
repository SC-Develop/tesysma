/*
 * stm32_lib_usart.h
 *
 * Created on:
 *      Author: Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef STM32LIBUSART_H_
#define STM32LIBUSART_H_

#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "libfifo.h"
#include "usart.def.h"

typedef enum  {
	USART_1 = 0,
	USART_2,
	USART_3,
	USART_4,
	USART_5,
	USART_6,
} USART_Id_TypeDef;

uint8_t USART_Init(void);

uint8_t USART_SetHandle(USART_Id_TypeDef id, UART_HandleTypeDef *husart);

HAL_StatusTypeDef USART_Start(USART_Id_TypeDef id);

Fifo_TypeDef *USART_RxFifo(USART_Id_TypeDef id);

void USART_TxFifoPushBuffer(USART_Id_TypeDef id, char *buff, uint16_t len);
void USART_TxFifoPushString(USART_Id_TypeDef id, char *string);

void USART_ReadChar(USART_Id_TypeDef id);

HAL_StatusTypeDef USART_Write(USART_Id_TypeDef id, char *mess, unsigned char blocking);
HAL_StatusTypeDef USART_Read(USART_Id_TypeDef id, char *mess, unsigned int len, unsigned char blocking);
HAL_StatusTypeDef USART_WriteChar(USART_Id_TypeDef id, char);

void USART_VSPrintf(USART_Id_TypeDef id, const char *fmt, va_list *argptr);
void USART_Printf(USART_Id_TypeDef id, const char *fmt,...);

#endif /* STM32LIBUSART_H_ */
