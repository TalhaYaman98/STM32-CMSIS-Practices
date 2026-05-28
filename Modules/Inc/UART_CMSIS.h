#ifndef UART_CMSIS_H
#define UART_CMSIS_H

#include "stm32f4xx.h"

void GPIO_UART_Init(void);
void USART2_Init(void);
void USART2_Send_Char(char c);
char USART2_Read_Char(void);
void USART2_Send_String(const char *str);

#endif /* UART_CMSIS_H */
