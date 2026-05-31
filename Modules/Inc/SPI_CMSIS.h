#ifndef SPI_CMSIS_H
#define SPI_CMSIS_H

#include "stm32f4xx.h"

void GPIO_SPI1_Init(void);
void SPI1_Init(void);
uint8_t SPI1_TransmitReceive(uint8_t data);
void SPI1_Transmit(uint8_t *pData, uint32_t size);
void SPI1_Receive(uint8_t *pData, uint32_t size);
void SPI1_CS_Low(void);
void SPI1_CS_High(void);

#endif /* SPI_CMSIS_H */
