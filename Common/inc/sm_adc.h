/**
 * @name sm_adc.h
 * @brief
 * @author
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef INC_SM_ANALOG_H_
#define INC_SM_ANALOG_H_

#include "libadc.h"
#include "sm_adc.def.h"

typedef enum {
    ADC_CHANNEL_BUSY,   // this means that a "stabilized" ADC value is not available yet
    ADC_CHANNEL_READY,   // this means ok to get channel value
	ADC_CHANNEL_ERROR,
} ADC_ChannelStatus_TypeDef;

/*
 * Module "public" interface. Usage:
 * - use this interface via public "VccInterface" instance
 * - call "Start" method to begin collecting analog data
 * - call "Stop" method to stopp collecting analog data
 * - query channel for valid data via "GetChannelStatus", data available if it returns VCC_CHANNEL_READY
 * - get channel value (actually a sliding mean) via "GetChannelValue"; returns 0xFFFF if channel not ready
 */
typedef struct {
    void (*Start)(void);
    void (*Stop) (void);
    void (*Exec) (void);
    uint16_t (*ChannelValue)(ADC_ChannelId_TypeDef channel);
    uint16_t (*ChannelVin)  (ADC_ChannelId_TypeDef channel, float *Vin);
    uint8_t  (*isStopped)   (void);
    void     (*Init)        (ADC_HandleTypeDef *hadc, float Vdd);
    ADC_ChannelStatus_TypeDef (*ChannelStatus)(ADC_ChannelId_TypeDef channel);
} ADCSmInterface_TypeDef;

// public interface singleton instance

extern ADCSmInterface_TypeDef * ADCInterface(void);

#endif /* INC_SM_ANALOG_H_ */
