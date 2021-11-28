/*
 * pdm_readings.c
 *
 *  Created on: Nov 20, 2021
 *      Author: Rodolfo
 */

#include "pdm.h"

//Changes multiplexer pins base on next data to be converted
//uint8_t next_data - next data to be converted:	0: Output current 0
//													1: Output current 1
//													2: Driver temperature
//													3: Driver voltage
static void PDM_Next_Data_Conversion(uint8_t next_data)
{
	switch(next_data)
	{
	case Data_Read_Current0:
		HAL_GPIO_WritePin(SEN_GPIO_Port, SEN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SEL0_GPIO_Port, SEL0_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(SEL1_GPIO_Port, SEL1_Pin, GPIO_PIN_RESET);
		break;

	case Data_Read_Current1:
		HAL_GPIO_WritePin(SEN_GPIO_Port, SEN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SEL0_GPIO_Port, SEL0_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(SEL1_GPIO_Port, SEL1_Pin, GPIO_PIN_SET);
		break;

	case Data_Read_Temperature:
		HAL_GPIO_WritePin(SEN_GPIO_Port, SEN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SEL0_GPIO_Port, SEL0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SEL1_GPIO_Port, SEL1_Pin, GPIO_PIN_RESET);
		break;

	case Data_Read_Voltage:
		HAL_GPIO_WritePin(SEN_GPIO_Port, SEN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SEL0_GPIO_Port, SEL0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(SEL1_GPIO_Port, SEL1_Pin, GPIO_PIN_SET);
		break;
	}

	return;
}

//Reads ADC value and converts into data
//uint8_t *data_read - data being read:	0: Output current 0
//										1: Output current 1
//										2: Driver temperature
//										3: Driver voltage
//Returns HAL_TIM_Base_Start_IT status
HAL_StatusTypeDef PDM_Read_Data(uint8_t *data_read)
{
	HAL_TIM_Base_Stop_IT(&htim7);

	switch(*data_read)
	{
	case Data_Read_Current0:
		*data_read = Data_Read_Current0;
		Accumulator_Delay = READING_DELAY_CURRENT1;

		for(uint8_t i = 0; i < 8; i++)
		{
			Data_Buffer[i * 2] = __PDM_CONVERT_CURRENT(ADC_BUFFER[i]);

			if(Data_Buffer[i * 2] > Output_Pin[i * 2].Current_Thresholds)
				Driver_Overcurrent_Flag |= (1 << (i * 2));

			if(ADC_BUFFER[i] < ADC_THRESHOLD_HIGH)
				Data_ID_Buffer[i * 2] |= 1;
			else
				Data_ID_Buffer[i * 2] &= 0xFFFE;
		}
		break;

	case Data_Read_Current1:
		*data_read = Data_Read_Temperature;
		Accumulator_Delay = READING_DELAY_TEMPERATURE;

		for(uint8_t i = 0; i < 8; i++)
		{
			Data_Buffer[(i * 2) + 1] = __PDM_CONVERT_CURRENT(ADC_BUFFER[i]);

			if(Data_Buffer[(i * 2) + 1] > Output_Pin[(i * 2) + 1].Current_Thresholds)
				Driver_Overcurrent_Flag |= (1 << ((i * 2) + 1));

			if(ADC_BUFFER[i] < ADC_THRESHOLD_HIGH)
				Data_ID_Buffer[(i * 2) + 1] |= 1;
			else
				Data_ID_Buffer[(i * 2) + 1] &= 0xFFFE;
		}
		break;

	case Data_Read_Temperature:
		*data_read = Data_Read_Voltage;
		Accumulator_Delay = READING_DELAY_VOLTAGE;

		for(uint8_t i = 0; i < 8; i++)
		{
			Data_Buffer[16 + i] = __PDM_CONVERT_TEMPERATURE(ADC_BUFFER[i], ADC_BUFFER[8]);

			if((ADC_BUFFER[i] < ADC_THRESHOLD_LOW) || (ADC_BUFFER[i] > ADC_THRESHOLD_HIGH))
				Data_ID_Buffer[16 + i] |= 1;
			else
				Data_ID_Buffer[16 + i] &= 0xFFFE;
		}
		break;

	case Data_Read_Voltage:
		*data_read = Data_Read_Current0;
		Accumulator_Delay = READING_DELAY_CURRENT0;

		for(uint8_t i = 0; i < 8; i++)
		{
			if((ADC_BUFFER[i] < ADC_THRESHOLD_LOW) || (ADC_BUFFER[i] > ADC_THRESHOLD_HIGH))
				Data_ID_Buffer[24] &= 0xFFFE;
			else
			{
				Data_Buffer[24] = __PDM_CONVERT_VOLTAGE(ADC_BUFFER[i], ADC_BUFFER[8]);
				Data_ID_Buffer[24] |= 1;
			}
		}

		if((Data_ID_Buffer[24] & 0xFFFE) == 0)
			Data_Buffer[0] = __PDM_CONVERT_VOLTAGE(ADC_BUFFER[0], ADC_BUFFER[8]);
		break;
	}

	Data_Buffer[25] = __PDM_CONVERT_MCU_TEMPERATURE(ADC_BUFFER[9]);

	PDM_Next_Data_Conversion(*data_read);

	return HAL_TIM_Base_Start_IT(&htim7);
}
