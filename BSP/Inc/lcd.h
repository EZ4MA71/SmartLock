#ifndef __LCD_H__
#define __LCD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32h7xx_hal.h"
#include "lcd_fonts.h"
#include "lcd_image.h"

/*----------------------------------------------- 参数宏 -------------------------------------------*/

#define LCD_Width     240       // LCD的像素长度
#define LCD_Height    320       // LCD的像素宽度

// 显示方向参数
#define Direction_H         0   // LCD横屏显示
#define Direction_H_Flip    1   // LCD横屏显示,上下翻转
#define Direction_V         2   // LCD竖屏显示
#define Direction_V_Flip    3   // LCD竖屏显示,上下翻转

// 设置变量显示时多余位补0还是补空格
#define Fill_Zero   0   // 填充0
#define Fill_Space  1   // 填充空格

/*---------------------------------------- 常用颜色 -----------------------------------------------*/

#define LCD_WHITE       0xFFFFFF    // 纯白色
#define LCD_BLACK       0x000000    // 纯黑色

#define LCD_BLUE        0x0000FF    // 纯蓝色
#define LCD_GREEN       0x00FF00    // 纯绿色
#define LCD_RED         0xFF0000    // 纯红色
#define LCD_CYAN        0x00FFFF    // 蓝绿色
#define LCD_MAGENTA     0xFF00FF    // 紫红色
#define LCD_YELLOW      0xFFFF00    // 黄色
#define LCD_GREY        0x2C2C2C    // 灰色

#define LIGHT_BLUE      0x8080FF    // 亮蓝色
#define LIGHT_GREEN     0x80FF80    // 亮绿色
#define LIGHT_RED       0xFF8080    // 亮红色
#define LIGHT_CYAN      0x80FFFF    // 亮蓝绿色
#define LIGHT_MAGENTA   0xFF80FF    // 亮紫红色
#define LIGHT_YELLOW    0xFFFF80    // 亮黄色
#define LIGHT_GREY      0xA3A3A3    // 亮灰色

#define DARK_BLUE       0x000080    // 暗蓝色
#define DARK_GREEN      0x008000    // 暗绿色
#define DARK_RED        0x800000    // 暗红色
#define DARK_CYAN       0x008080    // 暗蓝绿色
#define DARK_MAGENTA    0x800080    // 暗紫红色
#define DARK_YELLOW     0x808000    // 暗黄色
#define DARK_GREY       0x404040    // 暗灰色

/*--------------------------------------------- LCD其它引脚 ----------------------------------------*/

#define LCD_Backlight_PIN               GPIO_PIN_13             // 背光引脚
#define LCD_Backlight_PORT              GPIOE                   // 背光GPIO端口
#define GPIO_LDC_Backlight_CLK_ENABLE   __HAL_RCC_GPIOE_CLK_ENABLE()

#define LCD_Backlight_OFF   HAL_GPIO_WritePin(LCD_Backlight_PORT, LCD_Backlight_PIN, GPIO_PIN_RESET)
#define LCD_Backlight_ON    HAL_GPIO_WritePin(LCD_Backlight_PORT, LCD_Backlight_PIN, GPIO_PIN_SET)

#define LCD_DC_PIN          GPIO_PIN_15                         // 数据/指令选择引脚
#define LCD_DC_PORT         GPIOE                               // 数据/指令选择GPIO端口
#define GPIO_LDC_DC_CLK_ENABLE  __HAL_RCC_GPIOE_CLK_ENABLE()

#define LCD_DC_Command  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET)  // 低电平，指令
#define LCD_DC_Data     HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET)    // 高电平，数据

/*------------------------------------------------ 函数声明 -----------------------------------------*/

void  SPI_LCD_Init(void);
void  LCD_Clear(void);
void  LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

void  LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void  LCD_SetColor(uint32_t Color);
void  LCD_SetBackColor(uint32_t Color);
void  LCD_SetDirection(uint8_t direction);

void  LCD_SetAsciiFont(pFONT *fonts);
void  LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c);
void  LCD_DisplayString(uint16_t x, uint16_t y, char *p);

void  LCD_SetTextFont(pFONT *fonts);
void  LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText);
void  LCD_DisplayText(uint16_t x, uint16_t y, char *pText);

void  LCD_ShowNumMode(uint8_t mode);
void  LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number, uint8_t len);
void  LCD_DisplayDecimals(uint16_t x, uint16_t y, double number, uint8_t len, uint8_t decs);

void  LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);
void  LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height);
void  LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width);
void  LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void  LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void  LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r);
void  LCD_DrawEllipse(int x, int y, int r1, int r2);

void  LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void  LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r);

void  LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage);
void  LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *DataBuff);

void  LCD_Test_Clear(void);
void  LCD_Test_Text(void);
void  LCD_Test_Variable(void);
void  LCD_Test_Color(void);
void  LCD_Test_Grahic(void);
void  LCD_Test_Image(void);
void  LCD_Test_Direction(void);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H__ */
