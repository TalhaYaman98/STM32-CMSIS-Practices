
#include "DAC_CMSIS.h"

void GPIO_PA4_Analog_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;                    // GPIOA clock enable
    GPIOA->MODER |= (3 << (4 * 2));                         // PA4 → Analog mode (11)
}

void DAC_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;                      // DAC clock enable
    DAC->CR |= DAC_CR_EN1;                                  // DAC Channel 1 enable
}

// value: 0 - 4095 (12-bit)
void DAC_Write(uint16_t value) {
    if (value > 4095) value = 4095;                         // Limit kontrolü
    DAC->DHR12R1 = value;                                   // 12-bit right-aligned data
}

void DAC_Set_Voltage(float voltage) {
    // 3.3V referans varsayıldı → 0-3.3V arası
    uint16_t dac_val = (uint16_t)((voltage / 3.3f) * 4095);
    DAC_Write(dac_val);
}


