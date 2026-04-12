#ifndef __BSP_RTC_H
#define __BSP_RTC_H

#include "rtc.h"  // CubeMX 生成的，包含 hrtc 句柄

void    RTC_Init(void);
void    RTC_SetTime(uint8_t year, uint8_t month, uint8_t date,
					uint8_t hour, uint8_t min, uint8_t sec);
void    RTC_GetDateTime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);

#endif
