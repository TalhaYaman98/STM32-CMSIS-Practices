#ifndef WATCHDOG_CMSIS_H
#define WATCHDOG_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

/* IWDG - Independent Watchdog (LSI ~32 kHz) */
void IWDG_Init(uint8_t prescaler, uint16_t reload);
void IWDG_Refresh(void);

/* WWDG - Window Watchdog (APB1 clock) */
void WWDG_Init(uint8_t t_value, uint8_t w_value, uint8_t prescaler);
void WWDG_Refresh(uint8_t t_value);

#endif /* WATCHDOG_CMSIS_H */
