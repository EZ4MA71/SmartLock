#include "app_tasks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "log.h"
#include "lcd.h"

void vLedTask(void *pvParameters)
{
    LOG_INFO("LED task started");
    for (;;)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        LOG_INFO("LED1 ON / LED2 OFF");
        vTaskDelay(pdMS_TO_TICKS(1000));

        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        LOG_INFO("LED1 OFF / LED2 ON");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vDisplayTask(void *pvParameters)
{
    LOG_INFO("Display task started");

    SPI_LCD_Init();
    LOG_INFO("LCD init done");

    for (;;)
    {
        LCD_Test_Color();
        LOG_INFO("LCD test done, entering loop");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
