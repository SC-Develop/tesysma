/**
 * @file   timsys.h
 *
 * @date
 * @author Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef TIMER_SYS_H_
#define TIMER_SYS_H_

// Includes ***************************************************************************************

#include "main.h"

// Functions prototype ****************************************************************************

uint32_t TimSys_Time(void);
uint32_t TimSys_TimeElapsed(uint32_t start);
uint32_t TimSys_TickTimeElapsed(uint32_t *start, uint32_t timeout);
uint32_t TimSys_TickTimeElapsedEx(uint32_t *start, uint32_t timeout, uint8_t *start_from_now);

#endif /* TIMER_GEN_H_ */
