#ifndef __OLED_H
#define __OLED_H

#include "main.h"

// OLED显示模式
#define OLED_8X16 1
#define OLED_6X8  0

// 基本函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t mode);
void OLED_DrawPoint(uint8_t x, uint8_t y);
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_Printf(uint8_t x, uint8_t y, uint8_t mode, const char *fmt, ...);

// 图形函数声明
void OLED_InvertRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_FillRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_DrawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_SetContrast(uint8_t contrast);

// 显示控制函数
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);
void OLED_SetBrightness(uint8_t brightness);
void OLED_SetPosition(uint8_t x, uint8_t y);

// 绘图功能函数
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t radius);
void OLED_FillCircle(uint8_t x, uint8_t y, uint8_t radius);
void OLED_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height);

#endif 