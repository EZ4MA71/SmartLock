#include "bsp_rtc.h"

void RTC_Init(void)
{
	if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != RTC_MAGIC_NUM)
	{
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_MAGIC_NUM);
	}
}

void RTC_SetTime(uint8_t year, uint8_t month, uint8_t date,
				uint8_t hour, uint8_t min, uint8_t sec)
{
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	sTime.Hours   = hour;
	sTime.Minutes = min;
	sTime.Seconds = sec;
	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	sDate.Year  = year;
	sDate.Month = month;
	sDate.Date  = date;
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void RTC_GetDateTime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date)
{
	HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);  // 必须紧跟，释放锁
}
