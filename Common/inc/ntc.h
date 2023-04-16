/**
 * @file ntc.h
 * @date   Created on:
 * @author Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef NTC_FHT_H_
#define NTC_FHT_H_

#include "main.h"

// Defines ****************************************************************************************

#define T_REF 		298.15f  		  // Gradi K => 25°

// Parametri NTC1 Eq. Steinhart -------------------------------------------------------------------

#define NTC1_A 		7.91378234279882E-04f  	// Coeff. A Eq Steinhart
#define NTC1_B 		2.26539298535776E-04f  	// Coeff. B Eq Steinhart
#define NTC1_C 		0                		// Coeff. C Eq Steinhart
#define NTC1_D 		8.80540116411882E-08f  	// Coeff. D Eq Steinhart
#define NTC1_BETA 	3950                    // Coeff. Beta
#define NTC1_REF 	100000                  // Resistenza NTC alla temepratura di rif. (25°)
#define NTC1_RC  	100000                  // Resistenza di carico NTC
#define NTC1_VCC 	3.3                     // Tensione di alimentazione partitore

// Parametri NTC2 Eq. Steinhart -------------------------------------------------------------------

#define NTC2_A 		1.120046273E-03f  // Coeff. A Eq Steinhart
#define NTC2_B 		2.365778590E-04f  // Coeff. B Eq Steinhart
#define NTC2_C 		0                 // Coeff. C Eq Steinhart
#define NTC2_D		0.7040367192E-07f // Coeff. D Eq Steinhart
#define NTC2_BETA	3893.91  	      // Coeff. Beta
#define NTC2_REF	10000             // Resistenza NTC alla temepratura di rif. (25°)           // Coeff. Beta
#define NTC2_RC  	10000             // Resistenza di carico NTC
#define NTC2_VCC 	3.3               // Tensione di alimentazione partitore

// Types ******************************************************************************************

enum NTC_ID {
	NTC1 	= 0,
	NTC2 	= 1,
	NTC_MAX,
};

struct NTC {
	double A;      				// steinhart coef.
	double B;      				// steinhart coef.
	double D;  			    	// steinhart coef.
	double Beta;   				// coeff. BETA
	double Tref;   				// temperatura di riferimeto (Kelvin)
	double temp;   				// temp celsius calcolata
	double Rntc;   				// ntc resistance calcolata
	double adcv;   				// tensione rilevata dal convertirore ADC
	double vcc;    				// tensione alimentazione partitore.
	uint32_t Rc;   				// resistenza fissa partitore;
	uint32_t Rref; 				// resistenza del termistore alla temperature di riferimento 25 C°
	uint8_t  betaEnabled;
	uint8_t  bitRes;  			// ADC resosultion in bit (1,2,3...16 ...)
	uint16_t resFullScale;     	// ADC resolution full scale value es. for 12 bit => 4095 (12 bit ADC resolution = 4096)
};

// Function prototypes ****************************************************************************

void NTC_Init(uint16_t *adc_resFullScale);

float NTC_ABDTemp(struct NTC *ntc, uint32_t adc_value, uint16_t adc_fullscale);
float NTC_BTemp(struct NTC *ntc, uint32_t adc_value, uint16_t adc_fullscale);
float NTC_Temp(enum NTC_ID ntc, uint32_t adc_value);

struct NTC NTC_Get(enum NTC_ID ntc);
void NTC_Set(enum NTC_ID ntc,struct NTC *ntcparam);

int NTC_BetaEqEnabled(enum NTC_ID nct);
void NTC_EnableBetaEq(enum NTC_ID nct, uint8_t enabled);

#endif /* NTC_FHT_H_ */
