/**
 * @file   timsys.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "limits.h"
#include "timsys.h"

/******************************************************************************
 * SYS TIMER FUNCTIONS
 ******************************************************************************/

/**
 * @brief get the custom sys timer counter in timer unit from starting time
 */
uint32_t TimSys_Time(void)
{
	return HAL_GetTick();
}

/**
 * @brief get the sys time elapsed from provided time
 */
uint32_t TimSys_TimeElapsed(uint32_t start)
{
	uint32_t t = HAL_GetTick(); // get current sys time and freeze the value

	if (start > t) // timer has been reset
	{
	   return UINT_MAX - start + t;
	}

	return (t-start);
}

//* Tick time elapsed function ------------------------------------------------------------------------

/**
 * @brief Compute the timelapsed tick from last time tick (1 tick = 1 msec), the current time is computed by HAL_GetTick()
 *        if timeout is expired, start is set to readed tick end return the time_elapsed from start.
 *        On exit, if timeout expired, the 'time' var is updated to last tick readed by function,
 *        and ready for next computing.
 *        You should not need need to update explicitly this value becose is aready updated.
 *
 *        Typical usage:
 *
 *        	in your event responsive or iterated fuctions/code block
 *
 *        	{
 *         		static uint32_t time    = 0;
 * 		        uint32_t        timeout = 200; // [msec]
 *
 *      	    uint32_t timelapsed = TIMSys_TickTimeElapsed(&time, timeout);
 *
 *          	if (timelapsed)
 *          	{
 *             		// do action code
 *          	}
 *        	}
 *
 *        	or else (to prevent the timer timeout at first call)
 *
 *          {
 *         		static uint8_t  started = 0;
 *         		static uint32_t time    = 0;
 * 		        uint32_t        timeout = 200; // [msec]
 *
 *              if (time==0 && started ==0)
 *              {
 *               	started != started;
 *                  time    = HAL_GetTick();
  *             }
 *
 *      	    uint32_t timelapsed = TIMSys_TickTimeElapsed(&time, timeout);
 *
 *          	if (timelapsed)
 *          	{
 *             		// do action code
 *          	}
 *        	}
 *
 */
uint32_t TimSys_TickTimeElapsed(uint32_t *start, uint32_t timeout)
{
    uint32_t time_elapsed;
	uint32_t t = HAL_GetTick(); // get current sys time and freeze the value

	if (*start > t) // timer has been reset
	{
	   time_elapsed = UINT_MAX - (*start) + t;
	}
	else
	{
	   time_elapsed = t - (*start);
	}

	if (time_elapsed >= timeout)
	{
	   *start = t;
	   return time_elapsed;
	}

	return 0;
}

/**
 * @brief start the msec timer
 * @param start   		 => start tick
 * @param timeout 		 => timer timeout [msec]
 * @param start_from_now => if set, starts the time from current tick timer [msec]: start is set to current tick timer and, start_from_now unset
 *
 */
uint32_t TimSys_TickTimeElapsedEx(uint32_t *start, uint32_t timeout, uint8_t *start_from_now)
{
	if (*start_from_now)        // Start the timer
	{
		*start = HAL_GetTick(); // get current sys time and freeze the value

		*start_from_now = 0;    // to remember that the timer is already started
	}

	return TimSys_TickTimeElapsed(start, timeout);
}
