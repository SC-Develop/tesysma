/**
 * @file   stm32_lib_usart.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 */

#include "stm32_lib_usart.h"

// - Defines ------------------------------------------------------------------------------------------------- /

#define BUFFER_SIZE 256

#define USARTS_NUM    ((USART_MAX == 0) ? 1 : USART_MAX) // do not change:
#define USART_ID_MAX  USARTS_NUM - 1                      // do not change

// - Private variables --------------------------------------------------------------------------------------- /

static UART_HandleTypeDef *husart[USARTS_NUM];			// Managed USART handle

static char rx_char[USARTS_NUM];						// USART rx char

static char printfBuffer[BUFFER_SIZE];    				// USART2 internal printf buffer

static char rxBuffer[USARTS_NUM][FIFO_RX_BUFFER_SIZE];	// USART rx buffers managed by rx fifo
static char txBuffer[USARTS_NUM][FIFO_TX_BUFFER_SIZE];	// USART tx buffers managed by tx fifo

static Fifo_TypeDef rxFifo[USARTS_NUM];					// USART rx fifo
static Fifo_TypeDef txFifo[USARTS_NUM]; 				// USART tx fifo

// - Exported Functions -------------------------------------------------------------------------------------- /

/**
 * @fn void USART_Init(void)
 * @brief initialize USART Library: call this function before using USART_ library functions.
 *        All reference pointer with the previous USART Handle will be lost.
 *
 */
uint8_t USART_Init(void)
{
	memset(husart,0,sizeof(husart));

	uint8_t n = USARTS_NUM;

	while (n--)
	{
	    ch_fifo_init(txFifo + n, &txBuffer[n][0], FIFO_TX_BUFFER_SIZE);
		ch_fifo_init(rxFifo + n, rxBuffer[n], FIFO_RX_BUFFER_SIZE);
	}

    return 1;
}

/**
 * @fn void USART_SetHandle(USART_TypeDef, UART_HandleTypeDef*)
 * @brief associate the USART handle with the specified USART Id, for using with USART_ library.
 *
 * @param id
 * @param husart
 */
uint8_t USART_SetHandle(USART_Id_TypeDef id, UART_HandleTypeDef *huart)
{
	if (id > USART_ID_MAX)
	{
		return 0;
	}

	husart[id] = huart;

	return 1;
}

/**
 * @fn void USART2_InitQueue(void)
 * @brief start reading for Interrupt managed USART
 *
 */
HAL_StatusTypeDef USART_Start(USART_Id_TypeDef id)
{
   if (id > USART_ID_MAX)
   {
	   return 0;
   }

   return USART_Read(id, rx_char + id, 1, 0);    // read next char from usart
}

/**
 * @fn Fifo_TypeDef USART2_RxFifo*(void)
 * @brief
 *
 * @return
 */
Fifo_TypeDef *USART_RxFifo(USART_Id_TypeDef id)
{
	return rxFifo + id;
}

/**
 * @fn void USART2ReadNextChar(void)
 * @brief
 *
 */
void USART_ReadChar(USART_Id_TypeDef id)
{
   ch_fifo_push(rxFifo + id, *(rx_char + id));  // push last readed char into fifo

   /*if (strstr(rxFifo[id].buffer,"MO RING\r\n"))
   {
	  USART_Write(USART_2, "\r\nFound\r\n", 0);
   }*/

   USART_Read(id, rx_char + id, 1, 0);    		// read next char from usart
}

/**
 * @fn void USART2_WriteTxQueue(void)
 * @brief send tx fifo content to USART id
 *
 */
void USART_TxFifoSend(USART_Id_TypeDef id)
{
   char ch;

   while (ch_fifo_pop(txFifo + id, &ch))
   {
	  USART_WriteChar(id, ch);
   }
}

/**
 * @fn void USART2_QueueWrite(char*)
 * @brief push a string into USART2 tx queue
 *
 * @param null terminated string, if string lenght exceeds teh FIO_TX_BUFFER_LENGHT, the string will be truncated
 */
void USART_TxFifoPushString(USART_Id_TypeDef id, char *string)
{
   while (*string && ch_fifo_push(txFifo + id, *string++));
}

/**
 * @fn void USART2_TxFifoPushChars(char*, uint16_t)
 * @brief push a char buffer into USART2 tx queue
 *
 * @param string
 * @param len
 */
void USART_TxFifoPushBuffer(USART_Id_TypeDef id, char *buff, uint16_t len)
{
   while (len-- && ch_fifo_push(txFifo + id, *buff++));
}

/**
 * @brief write mess to usart 2
 * @param mess
 * @param blocking
 * @return operation code
 */
HAL_StatusTypeDef USART_Write(USART_Id_TypeDef id, char *mess, unsigned char blocking)
{
	if (blocking)
	{
		return HAL_UART_Transmit(*(husart + id),(unsigned char *) mess ,strlen(mess),HAL_MAX_DELAY);
	}

	return HAL_UART_Transmit_IT(*(husart + id),(unsigned char *) mess, strlen(mess));
 }

/**
 * @brief read mess from usart 2
 * @param mess
 * @param len
 * @param blocking
 * @return operation code
 */
HAL_StatusTypeDef USART_Read(USART_Id_TypeDef id, char *mess, unsigned int len, unsigned char blocking)
{
	if (blocking)
	{
	   return HAL_UART_Receive(*(husart + id),(unsigned char *)mess,len,HAL_MAX_DELAY);
	}

	return HAL_UART_Receive_IT(*(husart + id),(unsigned char *) mess, len);
 }

/**
 * @brief write a char on usart 2
 * @param c
 * @return operation code
 */
HAL_StatusTypeDef USART_WriteChar(USART_Id_TypeDef id, char c)
{
	char cc[2] = {c, 0};
	return USART_Write(id, cc, 1);
}

/**
 * @brief VSPrintf for usart 2
 * @param fmt
 * @param argptr
 * @return void
 */
void USART_VSPrintf(USART_Id_TypeDef id, const char *fmt, va_list *argptr)
{
    vsprintf((char *) printfBuffer, (const char *) fmt, *argptr);
    printfBuffer[BUFFER_SIZE-1] = 0;
    USART_Write(id, printfBuffer, 1);
}

/**
 * @brief Printf for usart 2
 * @param fmt
 * @param ...
 * @return void
 */
void USART_Printf(USART_Id_TypeDef id, const char *fmt,...)
{
	va_list argptr;
	va_start(argptr, fmt);
	USART_VSPrintf(id, fmt, &argptr);
	va_end(argptr);
}

//-- Interrupt callback weak functions ------------------------------------------------------------

__weak void USART1_RxCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART1_ErrorCallback(UART_HandleTypeDef *huart){};
__weak void USART1_AbortCpltCallback (UART_HandleTypeDef *huart){};
__weak void USART1_AbortTransmitCpltCallback (UART_HandleTypeDef *huart){};
__weak void USART1_AbortReceiveCpltCallback (UART_HandleTypeDef *huart){};
__weak void USART1_TxCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART1_TxHalfCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART1_RxHalfCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART2_RxCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART2_ErrorCallback(UART_HandleTypeDef *huart){};
__weak void USART2_AbortCpltCallback (UART_HandleTypeDef *huart){};
__weak void USART2_AbortTransmitCpltCallback (UART_HandleTypeDef *huart){};
__weak void USART2_AbortReceiveCpltCallback (UART_HandleTypeDef *huart){};
__weak void USART2_TxCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART2_TxHalfCpltCallback(UART_HandleTypeDef *huart){};
__weak void USART2_RxHalfCpltCallback(UART_HandleTypeDef *huart){};

//-------------------------------------------------------------------------------------------------

/**
 * @brief HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t n = USARTS_NUM;

	while(n--)
	{
		if (husart[n] == huart)
		{
			USART_ReadChar(n);

			if (huart->Instance==USART1)
			{
				USART1_RxCpltCallback(huart);
			}
			else
			if (huart->Instance==USART2)
			{
				USART2_RxCpltCallback(huart);
			}

			break;
		}
	}
}

/**
  * @brief UART error callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_ErrorCallback(huart);
	}
	else
	if (huart->Instance==USART2)
	{
		USART2_ErrorCallback(huart);
	}
}

/**
  * @brief Tx Transfer completed callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_TxCpltCallback(huart);
	}
	else
	if (huart->Instance==USART2)
	{
		USART2_TxCpltCallback(huart);
	}
}

/**
  * @brief  Tx Half Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_TxHalfCpltCallback(huart);
	}
	else
	if (huart->Instance==USART2)
	{
		USART2_TxHalfCpltCallback(huart);
	}
}

/**
  * @brief  Rx Half Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_RxHalfCpltCallback(huart);
	}
	else
	if (huart->Instance==USART2)
	{
		USART2_RxHalfCpltCallback(huart);
	}
}

/**
  * @brief  UART Abort Complete callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_AbortCpltCallback (UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_AbortCpltCallback(huart);
	}
	else
	if (huart->Instance==USART2)
	{
		USART2_AbortCpltCallback(huart);
	}
}

/**
  * @brief  UART Abort Complete callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_AbortTransmitCpltCallback (UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_AbortTransmitCpltCallback(huart);
	}
	else
	if (huart->Instance==USART2)
	{
		USART2_AbortTransmitCpltCallback(huart);
	}
}

/**
  * @brief  UART Abort Receive Complete callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_AbortReceiveCpltCallback (UART_HandleTypeDef *huart)
{
	if (huart->Instance==USART1)
	{
		USART1_AbortReceiveCpltCallback(huart);
	}
	else if (huart->Instance==USART2)
	{
		USART2_AbortReceiveCpltCallback(huart);
	}
}
