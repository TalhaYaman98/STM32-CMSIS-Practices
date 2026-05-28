#ifndef DAC_CMSIS_H
#define DAC_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

void GPIO_PA4_Analog_Init(void);
void DAC_Init(void);
void DAC_Write(uint16_t value);
void DAC_Set_Voltage(float voltage);

#endif /* DAC_CMSIS_H */
