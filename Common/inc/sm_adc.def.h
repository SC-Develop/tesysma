/*
 * sm_adc.def.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef INC_ADC_DEF_H_
#define INC_ADC_DEF_H_

typedef enum {
	CHN_NTC_TEMP    = 0,
	// CHN_TEMPSENSOR  = 1,
} ADC_ChannelId_TypeDef;

#define CHANNEL_COUNT           1
#define CIRCULAR_BUFFER_DIVISOR 4   // 2^7 = 128 byte buffer size => power of 2, establishes the circular buffer size

#endif /* INC_ADC_DEF_H_ */
