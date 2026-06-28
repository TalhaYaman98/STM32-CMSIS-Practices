#ifndef RTC_CMSIS_H
#define RTC_CMSIS_H

#include <stdint.h>
#include "stm32f4xx.h"

void RTC_Init(void);
void RTC_SetTime(uint8_t hour, uint8_t min, uint8_t sec);
void RTC_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday);
void RTC_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec);
void RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday);
void RTC_SetAlarm(uint8_t hour, uint8_t min, uint8_t sec);
void RTC_GetTimestamp(uint8_t *hour, uint8_t *min, uint8_t *sec);
void ALARM_IRQHandler(void);

#endif /* RTC_CMSIS_H */
