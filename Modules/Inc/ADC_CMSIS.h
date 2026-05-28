#ifndef ADC_CMSIS_H
#define ADC_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

void     GPIO_PA0_Analog_Init(void);
void     ADC1_Init(void);
uint16_t ADC1_Read(void);

extern uint16_t adc_value;

#endif /* ADC_CMSIS_H */
