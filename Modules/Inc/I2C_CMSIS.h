#ifndef I2C_CMSIS_H
#define I2C_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

void I2C1_Init(void);
void I2C1_Write(uint8_t slaveAddr, uint8_t *pData, uint32_t size);
void I2C1_WriteByte(uint8_t slaveAddr, uint8_t reg, uint8_t data);
void I2C1_ReadByte(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData);
void I2C1_ReadBytes(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint32_t size);

#endif /* I2C_CMSIS_H */
