/**
 * @brief  ntc.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "ntc.h"
#include "math.h"

static struct NTC ntc[] = {
	{ .A = NTC1_A, .B = NTC1_B, .D = NTC1_D, .Beta = NTC1_BETA, .Tref = T_REF, /*25, NTC1_REF, NTC1_VCC,*/ .vcc = 3.3, .Rc = NTC1_RC, .Rref = NTC1_REF, .betaEnabled = 1, .bitRes = 12, .resFullScale = 4095 },
	{ .A = NTC2_A, .B = NTC2_B, .D = NTC2_D, .Beta = NTC2_BETA, .Tref = T_REF, /*25, NTC2_REF, NTC2_VCC,*/ .vcc = 3.3, .Rc = NTC2_RC, .Rref = NTC2_REF, .betaEnabled = 1, .bitRes = 12, .resFullScale = 4095 }
};

// static int betaeq = 0;
/**
 * @fn void NTC_Init(uint16_t*)
 * @brief init the ADc resolotion for each NTC in array.
 *
 * @param adc_resolution array of ADC resolution for each NTC. The number of item MUST be NTC_MAX-1, and be NULL terminated
 */
void NTC_Init(uint16_t *adc_resFullScale)
{
	for (uint8_t n=0; n<NTC_MAX-1; n++)
	{
		ntc[n].resFullScale = *adc_resFullScale++;
	}
}

/**
 * @brief NTC_EnableBetaEq
 */
void NTC_EnableBetaEq(enum NTC_ID n, uint8_t enabled)
{
	ntc[n].betaEnabled = enabled;
}

/**
 * @brief NTC_BetaEqEnabled
 */
int NTC_BetaEqEnabled(enum NTC_ID n)
{
	return ntc[n].betaEnabled;
}

/**
 * @brief NTC_Set imposta i parametri per il calcolo della temperatura col metodo di Steinhart
 * @param parametri
 */
void NTC_Set(enum NTC_ID ntcid, struct NTC *ntcparam)
{
	ntc[ntcid] = *ntcparam;
}

/**
 * @brief NTC_Get return the NTC params
 * @return struct NTC
 */
struct NTC NTC_Get(enum NTC_ID ntcid)
{
	return ntc[ntcid];
}

/**
 * @brief NTC_ABDTemp    calcola la temperatura col metodo di Steinhart
 * @param adc_value      valore campionato all'ingresso del converitore ADC
 * @param adc_fullscale  fondo scala ADC usare ADCx_GetFullScale(VOID)
 *NTC_ABDTemp
 * Rthermistor = (ADC * Rfixed)/((Vref+)-ADC)
 */
float NTC_ABDTemp(struct NTC *ntc, uint32_t adc_value, uint16_t adc_fullscale)
{
	ntc->Rntc = (long) (((float) adc_value * ntc->Rc) / (adc_fullscale - (float) adc_value)); // Resistenza del termistore

	float fLog = (float) (log(ntc->Rntc));
	float fDenum = ntc->A + ntc->B * fLog + ntc->D * pow(fLog, 3);

	ntc->temp = (float) 1.0f / fDenum - 273.5;              // Celsius

	return ntc->temp;
}

/**
 * @brief NTC_BTemp
 */
float NTC_BTemp(struct NTC *ntc, uint32_t adc_value, uint16_t adc_fullscale)
{
	ntc->Rntc = (long) (((float) adc_value * ntc->Rc) / (adc_fullscale - (float) adc_value)); // Resistenza del termistore

	float fTemp = (float) (log(ntc->Rntc / ntc->Rref));
	float fDenum = ntc->Beta + fTemp * ntc->Tref;

	ntc->temp = (float) ((ntc->Beta * ntc->Tref) / (fDenum));
	ntc->temp -= 273.15;

	return ntc->temp;
}

/**
 * @brief NTC_TempCalc
 */
float NTC_Temp(enum NTC_ID ntcid, uint32_t adc_value)
{
	if ((ntc + ntcid)->betaEnabled)
	{
		return NTC_BTemp(ntc + ntcid, adc_value, (ntc + ntcid)->resFullScale);
	}

	return NTC_ABDTemp(ntc + ntcid, adc_value, (ntc + ntcid)->resFullScale);
}

