/**
 * @file   usart_Callback.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "stm32_lib_usart.h"

/**
 * @fn void USART1_RxCpltCallback(UART_HandleTypeDef*)
 * @brief  RECEIVING CHARACTER FROM SIM800L GSM MODULE
 *
 * @param huart
 */
void USART1_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char ch;

	Fifo_TypeDef *rxfifo = USART_RxFifo(USART_1);

	ch_fifo_get(rxfifo, &ch);

	USART_WriteChar(USART_2, ch);
};

/**
 * @fn void USART2_RxCpltCallback(UART_HandleTypeDef*)
 * @brief RECEIVING CHARACTER FROM SERIAL TERMINAL
 *
 * @param huart
 */
void USART2_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char ch;

	Fifo_TypeDef *rxfifo = USART_RxFifo(USART_2);

	ch_fifo_pop(rxfifo, &ch);

	USART_WriteChar(USART_1, ch); // send char to gsm module
};
