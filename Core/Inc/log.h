#ifndef __LOG_H__
#define __LOG_H__

#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

/* 直接用 HAL_UART_Transmit 发整行，不经过 printf */
static inline void LOG_Write(const char *buf, uint16_t len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
}

#define LOG_INFO(fmt, ...) do { \
    char _buf[128]; \
    int _len = snprintf(_buf, sizeof(_buf), \
                        "[%6lums][%-10s] " fmt "\r\n", \
                        xTaskGetTickCount(), \
                        pcTaskGetName(NULL), \
                        ##__VA_ARGS__); \
    LOG_Write(_buf, (uint16_t)_len); \
} while(0)

#define LOG_WARN(fmt, ...)  do { \
    char _buf[128]; \
    int _len = snprintf(_buf, sizeof(_buf), \
                        "[%6lums][%-10s] WARN: " fmt "\r\n", \
                        xTaskGetTickCount(), \
                        pcTaskGetName(NULL), \
                        ##__VA_ARGS__); \
    LOG_Write(_buf, (uint16_t)_len); \
} while(0)

#define LOG_ERROR(fmt, ...) do { \
    char _buf[128]; \
    int _len = snprintf(_buf, sizeof(_buf), \
                        "[%6lums][%-10s] ERR:  " fmt "\r\n", \
                        xTaskGetTickCount(), \
                        pcTaskGetName(NULL), \
                        ##__VA_ARGS__); \
    LOG_Write(_buf, (uint16_t)_len); \
} while(0)

  #endif
