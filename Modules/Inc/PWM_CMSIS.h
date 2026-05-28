#ifndef PWM_CMSIS_H
#define PWM_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

void GPIO_PD12_PWM_Init(void);
void TIM4_PWM_Init(uint32_t freq_hz, uint32_t duty);
void PWM_SetDutyCycle(uint32_t duty);

#endif /* PWM_CMSIS_H */
