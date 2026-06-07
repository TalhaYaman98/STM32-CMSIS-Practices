#ifndef DMA_CMSIS_H
#define DMA_CMSIS_H

#include "stm32f4xx.h"

void DMA_UART_Init(void);
void DMA_UART_Transmit(uint8_t *pData, uint16_t size);

void DMA_ADC_Init(void);
void DMA_ADC_Start(uint16_t *pData, uint16_t size);

#endif /* DMA_CMSIS_H */
