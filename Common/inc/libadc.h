/*
 * libadc.h
 *
 *  Created on:
 *      Author: Ing. Salvatore Cerami
 *
 *  @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef ADC_LIB_H_
#define ADC_LIB_H_

#include "main.h"

// _STM32F072 valore di riferimento interno di calibrazione della tensione
#define ADC_VREFINT ( *(uint16_t*)((uint32_t)0x1FFFF7BA) ) // Internal Reference Voltage for VDDA Calibration (VREFINT_CAL)

// STM32F446 valore di riferimento della tensione
// #define VREFINT ( (uint16_t*)((uint32_t)0x1FFF7A2A) )

#define ADC_VDD 3.3
#define ADC_NORMALIZE(adc_value,adc_full_scale) ( ( (float)(adc_value) )/(adc_full_scale) )
#define ADC_GET_VOLTAGE(adc_value,adc_full_scale,adc_vdd)  ( (adc_vdd) * ADC_NORMALIZE( (adc_value),(adc_full_scale) ) )

uint16_t ADC_GetFullScale(ADC_HandleTypeDef *hadc);
float   ADC_Normalize(uint16_t adc_value, uint16_t adc_full_scale);
float   ADC_CalcCalibratedVDDA(uint32_t adc_vrefint);
float   ADC_Rntc_Val(uint32_t adc_value, uint16_t Rc, uint16_t adc_fullscale);

#endif /* ADC_LIB_H_ */
