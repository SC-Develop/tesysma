/**
 * @file libadc.c - https://github.com/sc-develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include <libadc.h>

/**
 * @brief return a full-scale of ADC hdc (must be already initialized)
 * @param hadc
 * @return the full scale value of hadc.
 */
uint16_t ADC_GetFullScale(ADC_HandleTypeDef *hadc)
{
   switch(hadc->Init.Resolution)
   {
	   case ADC_RESOLUTION_12B:
	   return 4095;

	   case ADC_RESOLUTION_10B:
	   return 1023;

	   case ADC_RESOLUTION_8B:
	   return 255;

	   case ADC_RESOLUTION_6B:
	   return 63;
   }

   return 0;
}

/**
 * @brief normalize adc value to [0..1]
 * @param adc_value sholud be <= adc_resolution
 * @param adc_full_scale
 */
float ADC_Normalize(uint16_t adc_value, uint16_t adc_full_scale)
{
   return ((float)adc_value)/adc_full_scale;
}

/**
 * @brief
 * @param adc_value sholud be <= adc_resolution
 * @param adc_full_scale
 * @param adc_vref
 */
float ADC_Voltage(uint16_t adc_value, uint16_t adc_full_scale, float adc_vref)
{
   return adc_vref * ADC_NORMALIZE(adc_value,adc_full_scale);
}

/**
 * @brief Get VDD Calibrated value, This function is useful for calculatin current VDDA value.
 *        For using this function you should get the current vrefint value from ADC (channel vrefint).
 *        This function DO NOT acquire the vrefint form ADC. You must acquire vrefint before call this function.
 * @brief current vrefint ADC channel value
 *
 * return the calculated current VDDA value.
 */
float ADC_CalcCalibratedVDDA(uint32_t adc_vrefint)
{
   return ADC_VDD * ADC_VREFINT/adc_vrefint;
}

/**
 * @brief restituisce il valore della resistenza variabile del partitore di tensione corrispondente al
 *         valore letto dall'adc
 * @param adc_value       // valore letto dall'adc
 * @param Rc              // resistenza di carico del partiotre
 * @param adc_fullscale   // valore massimo di fondo scal dell'adc es. 4095 (12 bit) opp. 1024 (10 bit)
 */
float ADC_Rntc_Val(uint32_t adc_value, uint16_t Rc, uint16_t adc_fullscale)
{
    return  (float) adc_value * Rc/(adc_fullscale - (float)adc_value); // Resistenza del termistore
}


