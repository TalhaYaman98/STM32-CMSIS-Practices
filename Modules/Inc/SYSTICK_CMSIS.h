#ifndef SYSTICK_CMSIS_H
#define SYSTICK_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

extern volatile uint32_t tick_count;

void SysTick_Init(void);           // SysTick yapılandırması
void delay_ms(uint32_t ms);        // Milisaniye gecikme
void delay_us(uint32_t us);        // Mikrosaniye gecikme
uint32_t GetTick(void);            // Başlangıçtan bu yana geçen ms

#endif /* SYSTICK_CMSIS_H */
