
#ifndef FLASH_CMSIS_H_
#define FLASH_CMSIS_H_

#include <stdint.h>
#include "stm32f4xx.h"

/* Sektör Adresi Tablosu */
#define FLASH_SECTOR0_ADDR   0x08000000U   // 16 KB  — Bootloader/uygulama başlangıcı
#define FLASH_SECTOR1_ADDR   0x08004000U   // 16 KB
#define FLASH_SECTOR2_ADDR   0x08008000U   // 16 KB
#define FLASH_SECTOR3_ADDR   0x0800C000U   // 16 KB
#define FLASH_SECTOR4_ADDR   0x08010000U   // 64 KB
#define FLASH_SECTOR5_ADDR   0x08020000U   // 128 KB
#define FLASH_SECTOR6_ADDR   0x08040000U   // 128 KB
#define FLASH_SECTOR7_ADDR   0x08060000U   // 128 KB
#define FLASH_SECTOR8_ADDR   0x08080000U   // 128 KB
#define FLASH_SECTOR9_ADDR   0x080A0000U   // 128 KB
#define FLASH_SECTOR10_ADDR  0x080C0000U   // 128 KB
#define FLASH_SECTOR11_ADDR  0x080E0000U   // 128 KB

/* Flash Kilit / Kilit Açma */
void Flash_Unlock(void);
void Flash_Lock(void);

/* Sektör Silme */
void Flash_EraseSector(uint8_t sector);

/* Okuma */
uint32_t Flash_ReadWord(uint32_t address);
void Flash_ReadBuffer(uint32_t address, uint32_t *data, uint32_t length);

/* Yazma */
void Flash_WriteWord(uint32_t address, uint32_t data);
void Flash_WriteBuffer(uint32_t address, uint32_t *data, uint32_t length);

#endif /* FLASH_CMSIS_H_ */
