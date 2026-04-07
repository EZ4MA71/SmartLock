#include "lcd.h"
#include "spi.h"

#define LCD_SPI hspi4   // SPI移植宏，方便修改后换板移植

/* ----------------------------- 内部变量 ----------------------------------- */

static pFONT *LCD_AsciiFonts;   // 英文字体，ASCII字符库
static pFONT *LCD_CHFonts;      // 中文字体（同时也包含英文字体）

uint16_t LCD_Buff[1024];        // LCD缓冲区，16位色，每个像素占2字节

struct  // LCD相关参数结构体
{
    uint32_t Color;         // LCD当前画笔颜色
    uint32_t BackColor;     // 背景色
    uint8_t  ShowNum_Mode;  // 数字显示模式
    uint8_t  Direction;     // 显示方向
    uint16_t Width;         // 屏幕横向像素长度
    uint16_t Height;        // 屏幕纵向像素宽度
    uint8_t  X_Offset;      // X坐标偏移，用于调整屏幕不同摆放方式的读写方向
    uint8_t  Y_Offset;      // Y坐标偏移，用于调整屏幕不同摆放方式的读写方向
} LCD;

/* ----------------------------- 内部函数声明 -------------------------------- */

static HAL_StatusTypeDef LCD_SPI_WaitOnFlagUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag,
                                                         FlagStatus Status, uint32_t Tickstart,
                                                         uint32_t Timeout);
static void LCD_SPI_CloseTransfer(SPI_HandleTypeDef *hspi);
static HAL_StatusTypeDef LCD_SPI_Transmit(SPI_HandleTypeDef *hspi, uint16_t pData, uint32_t Size);
static HAL_StatusTypeDef LCD_SPI_TransmitBuffer(SPI_HandleTypeDef *hspi, uint16_t *pData, uint32_t Size);

/* ----------------------------- 底层 SPI 封装 ------------------------------- */

static void LCD_WriteCommand(uint8_t lcd_command)
{
    LCD_DC_Command;
    HAL_SPI_Transmit(&LCD_SPI, &lcd_command, 1, 1000);
}

static void LCD_WriteData_8bit(uint8_t lcd_data)
{
    LCD_DC_Data;
    HAL_SPI_Transmit(&LCD_SPI, &lcd_data, 1, 1000);
}

static void LCD_WriteData_16bit(uint16_t lcd_data)
{
    uint8_t lcd_data_buff[2];
    LCD_DC_Data;
    lcd_data_buff[0] = lcd_data >> 8;
    lcd_data_buff[1] = lcd_data;
    HAL_SPI_Transmit(&LCD_SPI, lcd_data_buff, 2, 1000);
}

static void LCD_WriteBuff(uint16_t *DataBuff, uint16_t DataSize)
{
    LCD_DC_Data;
    LCD_SPI.Init.DataSize = SPI_DATASIZE_16BIT;
    HAL_SPI_Init(&LCD_SPI);
    HAL_SPI_Transmit(&LCD_SPI, (uint8_t *)DataBuff, DataSize, 1000);
    LCD_SPI.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&LCD_SPI);
}

/* ----------------------------- 公共接口函数 --------------------------------- */

void SPI_LCD_Init(void)
{
    HAL_Delay(10);

    LCD_WriteCommand(0x36);
    LCD_WriteData_8bit(0x00);

    LCD_WriteCommand(0x3A);
    LCD_WriteData_8bit(0x05);

    LCD_WriteCommand(0xB2);
    LCD_WriteData_8bit(0x0C);
    LCD_WriteData_8bit(0x0C);
    LCD_WriteData_8bit(0x00);
    LCD_WriteData_8bit(0x33);
    LCD_WriteData_8bit(0x33);

    LCD_WriteCommand(0xB7);
    LCD_WriteData_8bit(0x35);

    LCD_WriteCommand(0xBB);
    LCD_WriteData_8bit(0x19);

    LCD_WriteCommand(0xC0);
    LCD_WriteData_8bit(0x2C);

    LCD_WriteCommand(0xC2);
    LCD_WriteData_8bit(0x01);

    LCD_WriteCommand(0xC3);
    LCD_WriteData_8bit(0x12);

    LCD_WriteCommand(0xC4);
    LCD_WriteData_8bit(0x20);

    LCD_WriteCommand(0xC6);
    LCD_WriteData_8bit(0x0F);

    LCD_WriteCommand(0xD0);
    LCD_WriteData_8bit(0xA4);
    LCD_WriteData_8bit(0xA1);

    LCD_WriteCommand(0xE0);
    LCD_WriteData_8bit(0xD0); LCD_WriteData_8bit(0x04); LCD_WriteData_8bit(0x0D);
    LCD_WriteData_8bit(0x11); LCD_WriteData_8bit(0x13); LCD_WriteData_8bit(0x2B);
    LCD_WriteData_8bit(0x3F); LCD_WriteData_8bit(0x54); LCD_WriteData_8bit(0x4C);
    LCD_WriteData_8bit(0x18); LCD_WriteData_8bit(0x0D); LCD_WriteData_8bit(0x0B);
    LCD_WriteData_8bit(0x1F); LCD_WriteData_8bit(0x23);

    LCD_WriteCommand(0xE1);
    LCD_WriteData_8bit(0xD0); LCD_WriteData_8bit(0x04); LCD_WriteData_8bit(0x0C);
    LCD_WriteData_8bit(0x11); LCD_WriteData_8bit(0x13); LCD_WriteData_8bit(0x2C);
    LCD_WriteData_8bit(0x3F); LCD_WriteData_8bit(0x44); LCD_WriteData_8bit(0x51);
    LCD_WriteData_8bit(0x2F); LCD_WriteData_8bit(0x1F); LCD_WriteData_8bit(0x1F);
    LCD_WriteData_8bit(0x20); LCD_WriteData_8bit(0x23);

    LCD_WriteCommand(0x21);

    LCD_WriteCommand(0x11);
    HAL_Delay(120);

    LCD_WriteCommand(0x29);

    LCD_SetDirection(Direction_V);
    LCD_SetBackColor(LCD_BLACK);
    LCD_SetColor(LCD_WHITE);
    LCD_Clear();

    LCD_SetAsciiFont(&ASCII_Font24);
    LCD_ShowNumMode(Fill_Zero);

    LCD_Backlight_ON;
}

void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_WriteCommand(0x2a);
    LCD_WriteData_16bit(x1 + LCD.X_Offset);
    LCD_WriteData_16bit(x2 + LCD.X_Offset);

    LCD_WriteCommand(0x2b);
    LCD_WriteData_16bit(y1 + LCD.Y_Offset);
    LCD_WriteData_16bit(y2 + LCD.Y_Offset);

    LCD_WriteCommand(0x2c);
}

void LCD_SetColor(uint32_t Color)
{
    uint16_t Red_Value   = (uint16_t)((Color & 0x00F80000) >> 8);
    uint16_t Green_Value = (uint16_t)((Color & 0x0000FC00) >> 5);
    uint16_t Blue_Value  = (uint16_t)((Color & 0x000000F8) >> 3);
    LCD.Color = (uint16_t)(Red_Value | Green_Value | Blue_Value);
}

void LCD_SetBackColor(uint32_t Color)
{
    uint16_t Red_Value   = (uint16_t)((Color & 0x00F80000) >> 8);
    uint16_t Green_Value = (uint16_t)((Color & 0x0000FC00) >> 5);
    uint16_t Blue_Value  = (uint16_t)((Color & 0x000000F8) >> 3);
    LCD.BackColor = (uint16_t)(Red_Value | Green_Value | Blue_Value);
}

void LCD_SetDirection(uint8_t direction)
{
    LCD.Direction = direction;

    if (direction == Direction_H) {
        LCD_WriteCommand(0x36);
        LCD_WriteData_8bit(0x70);
        LCD.X_Offset = 0;
        LCD.Y_Offset = 0;
        LCD.Width    = LCD_Height;
        LCD.Height   = LCD_Width;
    } else if (direction == Direction_V) {
        LCD_WriteCommand(0x36);
        LCD_WriteData_8bit(0x00);
        LCD.X_Offset = 0;
        LCD.Y_Offset = 0;
        LCD.Width    = LCD_Width;
        LCD.Height   = LCD_Height;
    } else if (direction == Direction_H_Flip) {
        LCD_WriteCommand(0x36);
        LCD_WriteData_8bit(0xA0);
        LCD.X_Offset = 0;
        LCD.Y_Offset = 0;
        LCD.Width    = LCD_Height;
        LCD.Height   = LCD_Width;
    } else if (direction == Direction_V_Flip) {
        LCD_WriteCommand(0x36);
        LCD_WriteData_8bit(0xC0);
        LCD.X_Offset = 0;
        LCD.Y_Offset = 0;
        LCD.Width    = LCD_Width;
        LCD.Height   = LCD_Height;
    }
}

void LCD_SetAsciiFont(pFONT *Asciifonts)
{
    LCD_AsciiFonts = Asciifonts;
}

void LCD_Clear(void)
{
    LCD_SetAddress(0, 0, LCD.Width - 1, LCD.Height - 1);
    LCD_DC_Data;
    LCD_SPI.Init.DataSize = SPI_DATASIZE_16BIT;
    HAL_SPI_Init(&LCD_SPI);
    LCD_SPI_Transmit(&LCD_SPI, LCD.BackColor, LCD.Width * LCD.Height);
    LCD_SPI.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&LCD_SPI);
}

void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    LCD_SetAddress(x, y, x + width - 1, y + height - 1);
    LCD_DC_Data;
    LCD_SPI.Init.DataSize = SPI_DATASIZE_16BIT;
    HAL_SPI_Init(&LCD_SPI);
    LCD_SPI_Transmit(&LCD_SPI, LCD.BackColor, width * height);
    LCD_SPI.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&LCD_SPI);
}

void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color)
{
    LCD_SetAddress(x, y, x, y);
    LCD_WriteData_16bit(color);
}

void LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c)
{
    uint16_t index = 0, counter = 0, i = 0, w = 0;
    uint8_t  disChar;

    c = c - 32;

    for (index = 0; index < LCD_AsciiFonts->Sizes; index++) {
        disChar = LCD_AsciiFonts->pTable[c * LCD_AsciiFonts->Sizes + index];
        for (counter = 0; counter < 8; counter++) {
            LCD_Buff[i] = (disChar & 0x01) ? LCD.Color : LCD.BackColor;
            disChar >>= 1;
            i++;
            w++;
            if (w == LCD_AsciiFonts->Width) {
                w = 0;
                break;
            }
        }
    }
    LCD_SetAddress(x, y, x + LCD_AsciiFonts->Width - 1, y + LCD_AsciiFonts->Height - 1);
    LCD_WriteBuff(LCD_Buff, LCD_AsciiFonts->Width * LCD_AsciiFonts->Height);
}

void LCD_DisplayString(uint16_t x, uint16_t y, char *p)
{
    while ((x < LCD.Width) && (*p != 0)) {
        LCD_DisplayChar(x, y, *p);
        x += LCD_AsciiFonts->Width;
        p++;
    }
}

void LCD_SetTextFont(pFONT *fonts)
{
    LCD_CHFonts = fonts;
    switch (fonts->Width) {
        case 12: LCD_AsciiFonts = &ASCII_Font12; break;
        case 16: LCD_AsciiFonts = &ASCII_Font16; break;
        case 20: LCD_AsciiFonts = &ASCII_Font20; break;
        case 24: LCD_AsciiFonts = &ASCII_Font24; break;
        case 32: LCD_AsciiFonts = &ASCII_Font32; break;
        default: break;
    }
}

void LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText)
{
    uint16_t i = 0, index = 0, counter = 0;
    uint16_t addr = 0;
    uint8_t  disChar;
    uint16_t Xaddress = 0;

    while (1) {
        if (*(LCD_CHFonts->pTable + i * LCD_CHFonts->Sizes + 0) == *pText &&
            *(LCD_CHFonts->pTable + i * LCD_CHFonts->Sizes + 1) == *(pText + 1)) {
            addr = i;
            break;
        }
        i++;
        if (i >= LCD_CHFonts->Table_Rows) break;
    }

    i = 0;
    for (index = 0; index < LCD_CHFonts->Sizes; index++) {
        disChar = *(LCD_CHFonts->pTable + addr * LCD_CHFonts->Sizes + index);
        for (counter = 0; counter < 8; counter++) {
            LCD_Buff[i] = (disChar & 0x01) ? LCD.Color : LCD.BackColor;
            i++;
            disChar >>= 1;
            Xaddress++;
            if (Xaddress == LCD_CHFonts->Width) {
                Xaddress = 0;
                break;
            }
        }
    }
    LCD_SetAddress(x, y, x + LCD_CHFonts->Width - 1, y + LCD_CHFonts->Height - 1);
    LCD_WriteBuff(LCD_Buff, LCD_CHFonts->Width * LCD_CHFonts->Height);
}

void LCD_DisplayText(uint16_t x, uint16_t y, char *pText)
{
    while (*pText != 0) {
        if (*pText <= 0x7F) {
            LCD_DisplayChar(x, y, *pText);
            x += LCD_AsciiFonts->Width;
            pText++;
        } else {
            LCD_DisplayChinese(x, y, pText);
            x += LCD_CHFonts->Width;
            pText += 2;
        }
    }
}

void LCD_ShowNumMode(uint8_t mode)
{
    LCD.ShowNum_Mode = mode;
}

void LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number, uint8_t len)
{
    char Number_Buffer[15];
    if (LCD.ShowNum_Mode == Fill_Zero)
        sprintf(Number_Buffer, "%0.*d", len, number);
    else
        sprintf(Number_Buffer, "%*d", len, number);
    LCD_DisplayString(x, y, (char *)Number_Buffer);
}

void LCD_DisplayDecimals(uint16_t x, uint16_t y, double decimals, uint8_t len, uint8_t decs)
{
    char Number_Buffer[20];
    if (LCD.ShowNum_Mode == Fill_Zero)
        sprintf(Number_Buffer, "%0*.*lf", len, decs, decimals);
    else
        sprintf(Number_Buffer, "%*.*lf", len, decs, decimals);
    LCD_DisplayString(x, y, (char *)Number_Buffer);
}

#define ABS(X)  ((X) > 0 ? (X) : -(X))

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
            yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, curpixel = 0;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x = x1;
    y = y1;

    if (x2 >= x1) { xinc1 = 1;  xinc2 = 1;  }
    else           { xinc1 = -1; xinc2 = -1; }

    if (y2 >= y1) { yinc1 = 1;  yinc2 = 1;  }
    else          { yinc1 = -1; yinc2 = -1; }

    if (deltax >= deltay) {
        xinc1 = 0; yinc2 = 0;
        den = deltax; num = deltax / 2; numadd = deltay; numpixels = deltax;
    } else {
        xinc2 = 0; yinc1 = 0;
        den = deltay; num = deltay / 2; numadd = deltax; numpixels = deltay;
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++) {
        LCD_DrawPoint(x, y, LCD.Color);
        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

void LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height)
{
    uint16_t i;
    for (i = 0; i < height; i++)
        LCD_Buff[i] = LCD.Color;
    LCD_SetAddress(x, y, x, y + height - 1);
    LCD_WriteBuff(LCD_Buff, height);
}

void LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width)
{
    uint16_t i;
    for (i = 0; i < width; i++)
        LCD_Buff[i] = LCD.Color;
    LCD_SetAddress(x, y, x + width - 1, y);
    LCD_WriteBuff(LCD_Buff, width);
}

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    LCD_DrawLine_H(x, y, width);
    LCD_DrawLine_H(x, y + height - 1, width);
    LCD_DrawLine_V(x, y, height);
    LCD_DrawLine_V(x + width - 1, y, height);
}

void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int Xadd = -r, Yadd = 0, err = 2 - 2 * r, e2;
    do {
        LCD_DrawPoint(x - Xadd, y + Yadd, LCD.Color);
        LCD_DrawPoint(x + Xadd, y + Yadd, LCD.Color);
        LCD_DrawPoint(x + Xadd, y - Yadd, LCD.Color);
        LCD_DrawPoint(x - Xadd, y - Yadd, LCD.Color);
        e2 = err;
        if (e2 <= Yadd) {
            err += ++Yadd * 2 + 1;
            if (-Xadd == Yadd && e2 <= Xadd) e2 = 0;
        }
        if (e2 > Xadd) err += ++Xadd * 2 + 1;
    } while (Xadd <= 0);
}

void LCD_DrawEllipse(int x, int y, int r1, int r2)
{
    int   Xadd = -r1, Yadd = 0, err = 2 - 2 * r1, e2;
    float K = 0, rad1 = r1, rad2 = r2;

    if (r1 > r2) {
        do {
            K = (float)(rad1 / rad2);
            LCD_DrawPoint(x - Xadd, y + (uint16_t)(Yadd / K), LCD.Color);
            LCD_DrawPoint(x + Xadd, y + (uint16_t)(Yadd / K), LCD.Color);
            LCD_DrawPoint(x + Xadd, y - (uint16_t)(Yadd / K), LCD.Color);
            LCD_DrawPoint(x - Xadd, y - (uint16_t)(Yadd / K), LCD.Color);
            e2 = err;
            if (e2 <= Yadd) {
                err += ++Yadd * 2 + 1;
                if (-Xadd == Yadd && e2 <= Xadd) e2 = 0;
            }
            if (e2 > Xadd) err += ++Xadd * 2 + 1;
        } while (Xadd <= 0);
    } else {
        Yadd = -r2;
        Xadd = 0;
        do {
            K = (float)(rad2 / rad1);
            LCD_DrawPoint(x - (uint16_t)(Xadd / K), y + Yadd, LCD.Color);
            LCD_DrawPoint(x + (uint16_t)(Xadd / K), y + Yadd, LCD.Color);
            LCD_DrawPoint(x + (uint16_t)(Xadd / K), y - Yadd, LCD.Color);
            LCD_DrawPoint(x - (uint16_t)(Xadd / K), y - Yadd, LCD.Color);
            e2 = err;
            if (e2 <= Xadd) {
                err += ++Xadd * 3 + 1;
                if (-Yadd == Xadd && e2 <= Yadd) e2 = 0;
            }
            if (e2 > Yadd) err += ++Yadd * 3 + 1;
        } while (Yadd <= 0);
    }
}

void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int32_t  D = 3 - (r << 1);
    uint32_t CurX = 0, CurY = r;

    while (CurX <= CurY) {
        if (CurY > 0) {
            LCD_DrawLine_V(x - CurX, y - CurY, 2 * CurY);
            LCD_DrawLine_V(x + CurX, y - CurY, 2 * CurY);
        }
        if (CurX > 0) {
            LCD_DrawLine_V(x - CurY, y - CurX, 2 * CurX);
            LCD_DrawLine_V(x + CurY, y - CurX, 2 * CurX);
        }
        if (D < 0)
            D += (CurX << 2) + 6;
        else {
            D += ((CurX - CurY) << 2) + 10;
            CurY--;
        }
        CurX++;
    }
    LCD_DrawCircle(x, y, r);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    LCD_SetAddress(x, y, x + width - 1, y + height - 1);
    LCD_DC_Data;
    LCD_SPI.Init.DataSize = SPI_DATASIZE_16BIT;
    HAL_SPI_Init(&LCD_SPI);
    LCD_SPI_Transmit(&LCD_SPI, LCD.Color, width * height);
    LCD_SPI.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&LCD_SPI);
}

void LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage)
{
    uint8_t  disChar;
    uint16_t Xaddress = x, Yaddress = y;
    uint16_t i = 0, j = 0, m = 0;
    uint16_t BuffCount = 0;
    uint16_t Buff_Height = (sizeof(LCD_Buff) / 2) / height;

    for (i = 0; i < height; i++) {
        for (j = 0; j < (float)width / 8; j++) {
            disChar = *pImage;
            for (m = 0; m < 8; m++) {
                LCD_Buff[BuffCount] = (disChar & 0x01) ? LCD.Color : LCD.BackColor;
                disChar >>= 1;
                Xaddress++;
                BuffCount++;
                if ((Xaddress - x) == width) {
                    Xaddress = x;
                    break;
                }
            }
            pImage++;
        }
        if (BuffCount == Buff_Height * width) {
            BuffCount = 0;
            LCD_SetAddress(x, Yaddress, x + width - 1, Yaddress + Buff_Height - 1);
            LCD_WriteBuff(LCD_Buff, width * Buff_Height);
            Yaddress = Yaddress + Buff_Height;
        }
        if ((i + 1) == height) {
            LCD_SetAddress(x, Yaddress, x + width - 1, i + y);
            LCD_WriteBuff(LCD_Buff, width * (i + 1 + y - Yaddress));
        }
    }
}

void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *DataBuff)
{
    LCD_SetAddress(x, y, x + width - 1, y + height - 1);
    LCD_DC_Data;
    LCD_SPI.Init.DataSize = SPI_DATASIZE_16BIT;
    HAL_SPI_Init(&LCD_SPI);
    LCD_SPI_TransmitBuffer(&LCD_SPI, DataBuff, width * height);
    LCD_SPI.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&LCD_SPI);
}

/* ----------------------------- 自定义 SPI 传输函数 -------------------------- */

static HAL_StatusTypeDef LCD_SPI_WaitOnFlagUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag,
                                                         FlagStatus Status, uint32_t Tickstart,
                                                         uint32_t Timeout)
{
    while ((__HAL_SPI_GET_FLAG(hspi, Flag) ? SET : RESET) == Status) {
        if ((((HAL_GetTick() - Tickstart) >= Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
            return HAL_TIMEOUT;
    }
    return HAL_OK;
}

static void LCD_SPI_CloseTransfer(SPI_HandleTypeDef *hspi)
{
    uint32_t itflag = hspi->Instance->SR;

    __HAL_SPI_CLEAR_EOTFLAG(hspi);
    __HAL_SPI_CLEAR_TXTFFLAG(hspi);
    __HAL_SPI_DISABLE(hspi);
    __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_TXP | SPI_IT_RXP | SPI_IT_DXP |
                                 SPI_IT_UDR | SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));
    CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

    if (hspi->State != HAL_SPI_STATE_BUSY_RX) {
        if ((itflag & SPI_FLAG_UDR) != 0UL) {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_UDR);
            __HAL_SPI_CLEAR_UDRFLAG(hspi);
        }
    }
    if (hspi->State != HAL_SPI_STATE_BUSY_TX) {
        if ((itflag & SPI_FLAG_OVR) != 0UL) {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
            __HAL_SPI_CLEAR_OVRFLAG(hspi);
        }
    }
    if ((itflag & SPI_FLAG_MODF) != 0UL) {
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_MODF);
        __HAL_SPI_CLEAR_MODFFLAG(hspi);
    }
    if ((itflag & SPI_FLAG_FRE) != 0UL) {
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FRE);
        __HAL_SPI_CLEAR_FREFLAG(hspi);
    }

    hspi->TxXferCount = (uint16_t)0UL;
    hspi->RxXferCount = (uint16_t)0UL;
}

static HAL_StatusTypeDef LCD_SPI_Transmit(SPI_HandleTypeDef *hspi, uint16_t pData, uint32_t Size)
{
    uint32_t tickstart;
    uint32_t Timeout = 1000;
    uint32_t LCD_pData_32bit;
    uint32_t LCD_TxDataCount;
    HAL_StatusTypeDef errorcode = HAL_OK;

    assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_TXONLY(hspi->Init.Direction));
    __HAL_LOCK(hspi);
    tickstart = HAL_GetTick();

    if (hspi->State != HAL_SPI_STATE_READY) {
        errorcode = HAL_BUSY;
        __HAL_UNLOCK(hspi);
        return errorcode;
    }
    if (Size == 0UL) {
        errorcode = HAL_ERROR;
        __HAL_UNLOCK(hspi);
        return errorcode;
    }

    hspi->State     = HAL_SPI_STATE_BUSY_TX;
    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
    LCD_TxDataCount = Size;
    LCD_pData_32bit = (pData << 16) | pData;

    hspi->pRxBuffPtr  = NULL;
    hspi->RxXferSize  = (uint16_t)0UL;
    hspi->RxXferCount = (uint16_t)0UL;
    hspi->TxISR       = NULL;
    hspi->RxISR       = NULL;

    if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
        SPI_1LINE_TX(hspi);

    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 0);
    __HAL_SPI_ENABLE(hspi);

    if (hspi->Init.Mode == SPI_MODE_MASTER)
        SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);

    while (LCD_TxDataCount > 0UL) {
        if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP)) {
            if ((hspi->TxXferCount > 1UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_01DATA)) {
                *((__IO uint32_t *)&hspi->Instance->TXDR) = (uint32_t)LCD_pData_32bit;
                LCD_TxDataCount -= (uint16_t)2UL;
            } else {
                *((__IO uint16_t *)&hspi->Instance->TXDR) = (uint16_t)pData;
                LCD_TxDataCount--;
            }
        } else {
            if ((((HAL_GetTick() - tickstart) >= Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U)) {
                LCD_SPI_CloseTransfer(hspi);
                __HAL_UNLOCK(hspi);
                SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
                hspi->State = HAL_SPI_STATE_READY;
                return HAL_ERROR;
            }
        }
    }

    if (LCD_SPI_WaitOnFlagUntilTimeout(hspi, SPI_SR_TXC, RESET, tickstart, Timeout) != HAL_OK)
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);

    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSUSP);
    if (LCD_SPI_WaitOnFlagUntilTimeout(hspi, SPI_FLAG_SUSP, RESET, tickstart, Timeout) != HAL_OK)
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);

    LCD_SPI_CloseTransfer(hspi);
    SET_BIT(hspi->Instance->IFCR, SPI_IFCR_SUSPC);
    __HAL_UNLOCK(hspi);
    hspi->State = HAL_SPI_STATE_READY;

    return (hspi->ErrorCode != HAL_SPI_ERROR_NONE) ? HAL_ERROR : errorcode;
}

static HAL_StatusTypeDef LCD_SPI_TransmitBuffer(SPI_HandleTypeDef *hspi, uint16_t *pData, uint32_t Size)
{
    uint32_t tickstart;
    uint32_t Timeout = 1000;
    uint32_t LCD_TxDataCount;
    HAL_StatusTypeDef errorcode = HAL_OK;

    assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_TXONLY(hspi->Init.Direction));
    __HAL_LOCK(hspi);
    tickstart = HAL_GetTick();

    if (hspi->State != HAL_SPI_STATE_READY) {
        errorcode = HAL_BUSY;
        __HAL_UNLOCK(hspi);
        return errorcode;
    }
    if (Size == 0UL) {
        errorcode = HAL_ERROR;
        __HAL_UNLOCK(hspi);
        return errorcode;
    }

    hspi->State     = HAL_SPI_STATE_BUSY_TX;
    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
    LCD_TxDataCount = Size;

    hspi->pRxBuffPtr  = NULL;
    hspi->RxXferSize  = (uint16_t)0UL;
    hspi->RxXferCount = (uint16_t)0UL;
    hspi->TxISR       = NULL;
    hspi->RxISR       = NULL;

    if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
        SPI_1LINE_TX(hspi);

    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 0);
    __HAL_SPI_ENABLE(hspi);

    if (hspi->Init.Mode == SPI_MODE_MASTER)
        SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);

    while (LCD_TxDataCount > 0UL) {
        if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP)) {
            if ((LCD_TxDataCount > 1UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_01DATA)) {
                *((__IO uint32_t *)&hspi->Instance->TXDR) = *((uint32_t *)pData);
                pData += 2;
                LCD_TxDataCount -= 2;
            } else {
                *((__IO uint16_t *)&hspi->Instance->TXDR) = *((uint16_t *)pData);
                pData += 1;
                LCD_TxDataCount--;
            }
        } else {
            if ((((HAL_GetTick() - tickstart) >= Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U)) {
                LCD_SPI_CloseTransfer(hspi);
                __HAL_UNLOCK(hspi);
                SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
                hspi->State = HAL_SPI_STATE_READY;
                return HAL_ERROR;
            }
        }
    }

    if (LCD_SPI_WaitOnFlagUntilTimeout(hspi, SPI_SR_TXC, RESET, tickstart, Timeout) != HAL_OK)
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);

    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSUSP);
    if (LCD_SPI_WaitOnFlagUntilTimeout(hspi, SPI_FLAG_SUSP, RESET, tickstart, Timeout) != HAL_OK)
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);

    LCD_SPI_CloseTransfer(hspi);
    SET_BIT(hspi->Instance->IFCR, SPI_IFCR_SUSPC);
    __HAL_UNLOCK(hspi);
    hspi->State = HAL_SPI_STATE_READY;

    return (hspi->ErrorCode != HAL_SPI_ERROR_NONE) ? HAL_ERROR : errorcode;
}

/* ----------------------------- 测试函数 ------------------------------------- */

void LCD_Test_Clear(void)
{
    uint8_t i = 0;
    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_BLACK);
    for (i = 0; i < 8; i++) {
        switch (i) {
            case 0: LCD_SetBackColor(LIGHT_RED);     break;
            case 1: LCD_SetBackColor(LIGHT_GREEN);   break;
            case 2: LCD_SetBackColor(LIGHT_BLUE);    break;
            case 3: LCD_SetBackColor(LIGHT_YELLOW);  break;
            case 4: LCD_SetBackColor(LIGHT_CYAN);    break;
            case 5: LCD_SetBackColor(LIGHT_GREY);    break;
            case 6: LCD_SetBackColor(LIGHT_MAGENTA); break;
            case 7: LCD_SetBackColor(LCD_WHITE);     break;
            default: break;
        }
        LCD_Clear();
        LCD_DisplayText(13,  70, "STM32H7 刷屏测试");
        LCD_DisplayText(13, 106, "屏幕分辨率:240*320");
        LCD_DisplayText(13, 142, "驱动芯片:ST7789");
        HAL_Delay(1000);
    }
}

void LCD_Test_Text(void)
{
    LCD_SetBackColor(LCD_BLACK);
    LCD_Clear();

    LCD_SetColor(LCD_WHITE);
    LCD_SetAsciiFont(&ASCII_Font32); LCD_DisplayString(0,  0, "!#$'()*+,-.0123");
    LCD_SetAsciiFont(&ASCII_Font24); LCD_DisplayString(0, 32, "!#$'()*+,-.012345678");
    LCD_SetAsciiFont(&ASCII_Font20); LCD_DisplayString(0, 56, "!#$'()*+,-.0123456789:;<");
    LCD_SetAsciiFont(&ASCII_Font16); LCD_DisplayString(0, 76, "!#$'()*+,-.0123456789:;<=>?@AB");
    LCD_SetAsciiFont(&ASCII_Font12); LCD_DisplayString(0, 92, "!#$'()*+,-.0123456789:;<=>?@ABCDEFGHIJKL");

    LCD_SetColor(LCD_CYAN);
    LCD_SetAsciiFont(&ASCII_Font12); LCD_DisplayString(0, 104, "!#&'()*+,-.0123456789:;<=>?@ABCDEFGHIJKL");
    LCD_SetAsciiFont(&ASCII_Font16); LCD_DisplayString(0, 116, "!#&'()*+,-.0123456789:;<=>?@AB");
    LCD_SetAsciiFont(&ASCII_Font20); LCD_DisplayString(0, 132, "!#&'()*+,-.0123456789:;<");
    LCD_SetAsciiFont(&ASCII_Font24); LCD_DisplayString(0, 152, "!#&'()*+,-.012345678");
    LCD_SetAsciiFont(&ASCII_Font32); LCD_DisplayString(0, 176, "!#&'()*+,-.0123");

    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_YELLOW);
    LCD_DisplayText(0, 216, "ASCII字符库");
    HAL_Delay(2000);
    LCD_Clear();

    LCD_SetTextFont(&CH_Font12); LCD_SetColor(0X8AC6D1); LCD_DisplayText(14,  10, "1212:MZXQ推荐");
    LCD_SetTextFont(&CH_Font16); LCD_SetColor(0XC5E1A5); LCD_DisplayText(14,  30, "1616:MZXQ推荐");
    LCD_SetTextFont(&CH_Font20); LCD_SetColor(0XFFB549); LCD_DisplayText(14,  60, "2020:MZXQ推荐");
    LCD_SetTextFont(&CH_Font24); LCD_SetColor(0XFF585D); LCD_DisplayText(14,  90, "2424:MZXQ推荐");
    LCD_SetTextFont(&CH_Font32); LCD_SetColor(0xFFB6B9); LCD_DisplayText(14, 130, "3232:MZXQ推荐");

    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_WHITE);
    LCD_DisplayText(14, 180, "中文显示");
    HAL_Delay(2000);
}

void LCD_Test_Variable(void)
{
    uint16_t i;
    int32_t  a = 0, b = 0, c = 0;
    double   p = 3.1415926, f = -1234.1234;

    LCD_SetBackColor(LCD_BLACK);
    LCD_Clear();

    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LIGHT_CYAN);
    LCD_DisplayText(0, 10, "正数:");
    LCD_DisplayText(0, 40, "负数:");

    LCD_SetColor(LIGHT_YELLOW);
    LCD_DisplayText(0,  80, "补空格:");
    LCD_DisplayText(0, 110, "补零:");

    LCD_SetColor(LIGHT_RED);
    LCD_DisplayText(0, 150, "小数:");
    LCD_DisplayText(0, 180, "小数:");

    for (i = 0; i < 100; i++) {
        LCD_SetColor(LIGHT_CYAN);
        LCD_ShowNumMode(Fill_Space);
        LCD_DisplayNumber(80, 10, b + i * 10, 4);
        LCD_DisplayNumber(80, 40, c - i * 10, 4);

        LCD_SetColor(LIGHT_YELLOW);
        LCD_ShowNumMode(Fill_Space);
        LCD_DisplayNumber(130,  80, a + i * 150, 8);
        LCD_ShowNumMode(Fill_Zero);
        LCD_DisplayNumber(130, 110, b + i * 150, 8);

        LCD_SetColor(LIGHT_RED);
        LCD_ShowNumMode(Fill_Space);
        LCD_DisplayDecimals(100, 150, p + i * 0.1,   6, 3);
        LCD_DisplayDecimals(100, 180, f + i * 0.01, 11, 4);

        HAL_Delay(15);
    }
    HAL_Delay(2500);
}

void LCD_Test_Color(void)
{
    uint16_t i, y;

    LCD_SetBackColor(LCD_BLACK);
    LCD_Clear();

    LCD_SetTextFont(&CH_Font20);
    LCD_SetColor(LCD_WHITE);
    LCD_DisplayText(0, 0, "RGB Color:");

    for (i = 0; i < 240; i++) { LCD_SetColor(LCD_RED   - (i << 16)); LCD_DrawLine_V(i, 20, 10); }
    for (i = 0; i < 240; i++) { LCD_SetColor(LCD_GREEN - (i <<  8)); LCD_DrawLine_V(i, 35, 10); }
    for (i = 0; i < 240; i++) { LCD_SetColor(LCD_BLUE  - i);         LCD_DrawLine_V(i, 50, 10); }

    y = 70;
    LCD_SetColor(LIGHT_CYAN);    LCD_FillRect(150, y +  5,      90, 10); LCD_DisplayString(0, y,        "LIGHT_CYAN");
    LCD_SetColor(LIGHT_MAGENTA); LCD_FillRect(150, y + 20 + 5,  90, 10); LCD_DisplayString(0, y + 20,   "LIGHT_MAGENTA");
    LCD_SetColor(LIGHT_YELLOW);  LCD_FillRect(150, y + 40 + 5,  90, 10); LCD_DisplayString(0, y + 40,   "LIGHT_YELLOW");
    LCD_SetColor(LIGHT_GREY);    LCD_FillRect(150, y + 60 + 5,  90, 10); LCD_DisplayString(0, y + 60,   "LIGHT_GREY");
    LCD_SetColor(LIGHT_RED);     LCD_FillRect(150, y + 80 + 5,  90, 10); LCD_DisplayString(0, y + 80,   "LIGHT_RED");
    LCD_SetColor(LIGHT_BLUE);    LCD_FillRect(150, y + 100 + 5, 90, 10); LCD_DisplayString(0, y + 100,  "LIGHT_BLUE");

    LCD_SetColor(DARK_CYAN);     LCD_FillRect(150, y + 120 + 5, 90, 10); LCD_DisplayString(0, y + 120,  "DARK_CYAN");
    LCD_SetColor(DARK_MAGENTA);  LCD_FillRect(150, y + 140 + 5, 90, 10); LCD_DisplayString(0, y + 140,  "DARK_MAGENTA");
    LCD_SetColor(DARK_YELLOW);   LCD_FillRect(150, y + 160 + 5, 90, 10); LCD_DisplayString(0, y + 160,  "DARK_YELLOW");
    LCD_SetColor(DARK_GREY);     LCD_FillRect(150, y + 180 + 5, 90, 10); LCD_DisplayString(0, y + 180,  "DARK_GREY");
    LCD_SetColor(DARK_RED);      LCD_FillRect(150, y + 200 + 5, 90, 10); LCD_DisplayString(0, y + 200,  "DARK_RED");
    LCD_SetColor(DARK_GREEN);    LCD_FillRect(150, y + 220 + 5, 90, 10); LCD_DisplayString(0, y + 220,  "DARK_GREEN");

    HAL_Delay(2000);
}

void LCD_Test_Grahic(void)
{
    LCD_SetBackColor(LCD_BLACK);
    LCD_Clear();

    LCD_SetColor(LCD_WHITE);   LCD_DrawRect(0, 0, 240, 320);
    LCD_SetColor(LCD_RED);     LCD_FillCircle(140, 50, 30);
    LCD_SetColor(LCD_GREEN);   LCD_FillCircle(170, 50, 30);
    LCD_SetColor(LCD_BLUE);    LCD_FillCircle(200, 50, 30);

    LCD_SetColor(LCD_YELLOW);
    LCD_DrawLine(26,  26, 113, 64);
    LCD_DrawLine(35,  22, 106, 81);
    LCD_DrawLine(45,  20,  93, 100);
    LCD_DrawLine(52,  16,  69, 108);
    LCD_DrawLine(62,  16,  44, 108);

    LCD_SetColor(LIGHT_CYAN);
    LCD_DrawCircle(120, 170, 30);
    LCD_DrawCircle(120, 170, 20);

    LCD_SetColor(LIGHT_RED);
    LCD_DrawEllipse(120, 170, 90, 40);
    LCD_DrawEllipse(120, 170, 70, 40);
    LCD_SetColor(LIGHT_MAGENTA);
    LCD_DrawEllipse(120, 170, 100, 50);
    LCD_DrawEllipse(120, 170, 110, 60);

    LCD_SetColor(DARK_RED);    LCD_FillRect( 50, 250, 50, 50);
    LCD_SetColor(DARK_BLUE);   LCD_FillRect(100, 250, 50, 50);
    LCD_SetColor(LIGHT_GREEN); LCD_FillRect(150, 250, 50, 50);

    HAL_Delay(2000);
}

void LCD_Test_Image(void)
{
    LCD_SetBackColor(LCD_BLACK);
    LCD_Clear();

    LCD_SetColor(0xffF6E58D); LCD_DrawImage( 19,  55, 83, 83, Image_Android_83x83);
    LCD_SetColor(0xffDFF9FB); LCD_DrawImage(141,  55, 83, 83, Image_Message_83x83);
    LCD_SetColor(0xff9DD3A8); LCD_DrawImage( 19, 175, 83, 83, Image_Toys_83x83);
    LCD_SetColor(0xffFF8753); LCD_DrawImage(141, 175, 83, 83, Image_Video_83x83);

    HAL_Delay(2000);
}

void LCD_Test_Direction(void)
{
    for (int i = 0; i < 4; i++) {
        LCD_SetBackColor(LCD_BLACK);
        LCD_Clear();
        LCD_SetTextFont(&CH_Font24);
        LCD_SetColor(0xffDFF9FB);
        switch (i) {
            case 0: LCD_SetDirection(Direction_V);      LCD_DisplayText(20, 20, "Direction_V");      break;
            case 1: LCD_SetDirection(Direction_H);      LCD_DisplayText(20, 20, "Direction_H");      break;
            case 2: LCD_SetDirection(Direction_V_Flip); LCD_DisplayText(20, 20, "Direction_V_Flip"); break;
            case 3: LCD_SetDirection(Direction_H_Flip); LCD_DisplayText(20, 20, "Direction_H_Flip"); break;
            default: break;
        }
        LCD_SetColor(0xffF6E58D);
        LCD_DrawImage(19, 80, 83, 83, Image_Android_83x83);
        LCD_SetTextFont(&CH_Font32);
        LCD_SetColor(0xff9DD3A8);
        LCD_DisplayText(130,  90, "MZXQ");
        LCD_DisplayText(130, 130, "推荐");
        LCD_SetDirection(Direction_V);
        HAL_Delay(1000);
    }
}
