/**
 * @file sm_adc.c - https://github.com/SC-Develop/tesysma
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/SC-Develop/
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#include "main.h"
#include "adc.h"
#include "sm_adc.h"

/**
 * Defines *************************************************************************************************** /
 */

#define INTER_CHANNEL_DELAY     1   // time (ms) between subsequent channel acquisitions
#define CIRCULAR_BUFFER_SIZE    (uint8_t) (1 << CIRCULAR_BUFFER_DIVISOR)
#define ADC_WAIT_TIMEOUT        1000

/**
 * Type Definitions ****************************************************************************************** /
 */

/**
 * @enum
 * @brief
 *
 */
typedef enum
{
	ADC_SM_IDLE = 0, /**< ADC_SM_IDLE */
	ADC_SM_STOP,
	ADC_SM_START,
	ADC_SM_START_CONVERSION, /**< ADC_SM_START_CONVERSION */
	ADC_SM_WAITING_FOR_COMPLETE,
	ADC_SM_CONVERSION_COMPLETED,/**< ADC_SM_CONVERSION_COMPLETED */
	ADC_SM_CHANNEL_DELAY, /**< ADC_SM_CHANNEL_DELAY */
} ADCSmStatus_TypeDef;

/**
 * @struct
 * @brief
 *
 */
typedef struct
{
	uint32_t Channel;
	uint32_t Rank;
	uint32_t SamplingTime;
	uint32_t Buffer[CIRCULAR_BUFFER_SIZE];
	uint8_t  BufferIndex;
	ADC_ChannelStatus_TypeDef ChannelStatus;
} ADC_Channel_TypeDef;

/**
 * @struct
 * @brief
 *
 */
typedef struct
{
	float 				  Vdd;
	uint16_t              adc_full_scale;
	ADC_HandleTypeDef     *hadc;
	uint8_t               AvgSample;
	ADC_ChannelId_TypeDef CurrentChannel;
	ADCSmStatus_TypeDef   Status;
} ADC_StateMachine;

/**
 * Functions Prototypes **************************************************************************************** /
 */

static void smInit(ADC_HandleTypeDef *hadc, float Vdd);
static void smStart(void);
static void smStop(void);
static void smExec(void);
static ADC_ChannelStatus_TypeDef smChannelStatus(ADC_ChannelId_TypeDef channel);
static uint16_t smChannelValue(ADC_ChannelId_TypeDef channel);
static uint16_t smChannelVin(ADC_ChannelId_TypeDef channel, float *Vin);
static uint8_t  smIsStopped(void);

/**
 * Local Variables ********************************************************************************************* /
 */

/*
 * ADC Channel Map
 */
static ADC_Channel_TypeDef Channels[CHANNEL_COUNT] = {

	[CHN_NTC_TEMP] = {.Channel = ADC_CHANNEL_1, .Rank = 1, .SamplingTime = ADC_SAMPLETIME_15CYCLES, .ChannelStatus = ADC_CHANNEL_BUSY, .BufferIndex = 0, .Buffer = {0}},

};

static ADC_StateMachine StateMachine = {.CurrentChannel = 0, .Status = ADC_SM_IDLE};

/*
 * ADC Interface
 */
ADCSmInterface_TypeDef Interface = {
	.Start 		   = smStart,
	.Stop 		   = smStop,
	.Exec   	   = smExec,
	.ChannelStatus = smChannelStatus,
	.ChannelValue  = smChannelValue,
	.ChannelVin    = smChannelVin,
	.isStopped     = smIsStopped,
	.Init          = smInit,
};

/**
 * Local Functions ********************************************************************************************* /
 */
static uint8_t smIsStopped(void)
{
	return (StateMachine.Status == ADC_SM_IDLE);
}

/**
 * @fn void smInit(void)
 * @brief
 *
 */
static void smInit(ADC_HandleTypeDef *hadc, float Vdd)
{
	StateMachine.hadc           = hadc;
	StateMachine.Vdd            = Vdd;
	StateMachine.adc_full_scale = ADC_GetFullScale(StateMachine.hadc);
	StateMachine.Status         = ADC_SM_IDLE;
}

/**
 * @name StartHandler
 * @brief initialize channel buffer, then start ADC conversion on all channels.
 *
 * ADC conversion is programmed in continuous mode.
 */
static void smStart(void)
{
	StateMachine.Status = ADC_SM_START;
}

/**
 * @name StopHandler
 * @brief stop ADC conversion for both ADCs
 */
static void smStop(void)
{
	StateMachine.Status = ADC_SM_STOP;
}

/**
 * @name AddValueToChannel
 * @brief add specified value to the circular buffer associated to the given channel
 *
 * Values are managed in a ring-buffer, because the channel value is actually the arithmetic mean of the buffer itself
 */
static void AddValueToChannel(ADC_Channel_TypeDef *channel, uint32_t value)
{
	channel->Buffer[channel->BufferIndex] = value;

	channel->BufferIndex += 1;

	if (channel->BufferIndex >= CIRCULAR_BUFFER_SIZE)
	{
		channel->BufferIndex   = 0;
		channel->ChannelStatus = ADC_CHANNEL_READY;
	}
}

/**
 * @name SelectADCChannel
 * @brief Select or de-select a given ADC channel for next conversion
 *
 * if flag == SET, the selected ADC channel is selected for conversion, actual conversion
 * will happen on HAL_ADC_Start_IT.
 */
static void SelectADCChannel(ADC_HandleTypeDef *hadc, ADC_Channel_TypeDef *channel)
{
	ADC_ChannelConfTypeDef sConfig = {0};

	sConfig.Channel      = channel->Channel;
	sConfig.SamplingTime = channel->SamplingTime;
	sConfig.Rank         = channel->Rank;

	if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
	{
	  	channel->ChannelStatus = ADC_CHANNEL_ERROR;
	}
	else
	{
		channel->ChannelStatus = ADC_CHANNEL_BUSY;
	}
}

/**
 * @name RunHandler
 * @brief State machine run handler
 *
 * Actual ADC channel conversion happens here, provided that this handler gets called repeatedly.
 * Nothing happens until StateMachine.EnableChannelScan == SET (start handler invoked). When channel scan is
 * enabled, every channel listed in Channels array is prepared for conversion, according to the
 * specified parameters. ADC conversion is run in interrupt mode, so that the "conversion completed
 * callback" is called at end of conversion, and the analog value is stored in the selected channel
 * buffer. Then, conversion starts on the next analog line, after a period of INTER_CHANNEL_DELAY.
 * The analog interface handler does not deliver a value until the channel buffer (managed as a
 * ring buffer) is completely filled with analog data; when it gets filled, the analog value is the
 * mean value of the channel buffer, so that noise on analog lines is filtered away.
 */
static void smExec(void)
{
	static uint32_t Timeout;

	ADC_Channel_TypeDef *channel = Channels + StateMachine.CurrentChannel;

	switch (StateMachine.Status)
	{
		case ADC_SM_IDLE:
			// do nothing
		break;

		case ADC_SM_STOP:

			HAL_ADC_Stop_IT(StateMachine.hadc);

			StateMachine.Status = ADC_SM_IDLE;

		break;

		case ADC_SM_START:

			for (uint8_t i = 0; i < CHANNEL_COUNT; i++)
			{
				Channels[i].ChannelStatus = ADC_CHANNEL_BUSY;
				Channels[i].BufferIndex   = 0;

				for (int j = 0; j < CIRCULAR_BUFFER_SIZE; j++)
				{
					Channels[i].Buffer[j] = 0;
				}
			}

			StateMachine.CurrentChannel = 0;
			StateMachine.AvgSample     = 0;

			StateMachine.Status = ADC_SM_START_CONVERSION;

		break;

		case ADC_SM_START_CONVERSION:

			SelectADCChannel(StateMachine.hadc, channel);

			HAL_ADC_Start_IT(StateMachine.hadc);

			Timeout = HAL_GetTick() + ADC_WAIT_TIMEOUT;

			StateMachine.Status = ADC_SM_WAITING_FOR_COMPLETE;

		break;

		case ADC_SM_WAITING_FOR_COMPLETE:

			if (HAL_GetTick() >= Timeout)
			{
				smStop();
			}

		break;

		case ADC_SM_CONVERSION_COMPLETED:
		{
			// SelectADCChannel(StateMachine.hadc, channel); // de-select channel rank

			uint32_t value = HAL_ADC_GetValue(StateMachine.hadc); // read channel value

			AddValueToChannel(channel, value);

			if (++StateMachine.CurrentChannel == CHANNEL_COUNT)
			{
				StateMachine.CurrentChannel = 0;

				if (++StateMachine.AvgSample == 16)
				{
					StateMachine.Status = ADC_SM_IDLE; // conversione e media di tutti i canali completata
				}
			}
			else
			{
				Timeout = HAL_GetTick() + INTER_CHANNEL_DELAY;

				StateMachine.Status = ADC_SM_CHANNEL_DELAY;
			}
		}
		break;

		case ADC_SM_CHANNEL_DELAY:

			if (HAL_GetTick() >= Timeout)
			{
				StateMachine.Status = ADC_SM_START_CONVERSION;
			}

		break;
	}
}

/**
 * @fn ADC_ChannelStatus_TypeDef smChannelStatus(ChannelId_TypeDef)
 * @brief
 *
 * @param channel
 * @return
 */
static ADC_ChannelStatus_TypeDef smChannelStatus(ADC_ChannelId_TypeDef channel)
{
	if (channel >= 0 && channel < CHANNEL_COUNT)
	{
		return Channels[channel].ChannelStatus;
	}

	return ADC_CHANNEL_ERROR;
}

/**
 * @name GetChannelValue
 * @brief return mean value of the specified channel
 *
 * The channel value is actually the computed mean of the values in the circular buffer.
 * If the channel is not "ready" (never started ADC conversion, or buffer still being filled),
 * should not be queried, but in case, the returned value will be 0xFFFF.
 */
static uint16_t smChannelValue(ADC_ChannelId_TypeDef channel)
{
	if (channel >= 0 && channel < CHANNEL_COUNT)
	{
		if (Channels[channel].ChannelStatus == ADC_CHANNEL_READY)
		{
			uint32_t mean = 0;

			for (int i = 0; i < CIRCULAR_BUFFER_SIZE; i++)
			{
				mean += Channels[channel].Buffer[i];
			}

			mean >>= CIRCULAR_BUFFER_DIVISOR;

			return (uint16_t) (mean & 0x0FFF); // channel configured for 12 bit, strip excess bits
		}
	}

	return 0x0FFFF;
}

/**
 * @fn uint16_t ChannelVin(ADC_ChannelId_TypeDef, float*)
 * @brief
 *
 * @param channel
 * @param Vin
 * @return
 */
static uint16_t smChannelVin(ADC_ChannelId_TypeDef channel, float *Vin)
{
	uint16_t value = smChannelValue(channel);

	if (value != 0xFFFF)
	{
		*Vin = ADC_GET_VOLTAGE(value, StateMachine.adc_full_scale, StateMachine.Vdd);
	}

	return value;
}

/**
 * Exported functions ******************************************************************************************** /
 */

/**
 * @fn ADCSmInterface_TypeDef ADCInterface*(void)
 * @brief
 *
 * @return
 */
ADCSmInterface_TypeDef * ADCInterface(void)
{
	return &Interface;
}

/**
 * @name HAL_ADC_ConvCpltCallback
 * @brief ADC channel conversion complete callback
 *
 * This function must be visible from outside, it's an overload of a weak function
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	UNUSED(hadc);

	StateMachine.Status = ADC_SM_CONVERSION_COMPLETED;
}
