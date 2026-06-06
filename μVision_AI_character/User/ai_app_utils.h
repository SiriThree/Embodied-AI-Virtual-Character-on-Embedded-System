#ifndef __AI_APP_UTILS_H
#define __AI_APP_UTILS_H

#include "ai_app_data.h"

const char *CopyLineByPixelWidth(const char *src, char *dst, uint16_t max_pixel_width);
uint16_t TextPixelWidth(const char *text);
void TrimLineWithEllipsis(char *text, uint16_t max_pixel_width);
uint8_t WrapTextLines(const char *src, char lines[][MAX_TEXT_LEN], uint8_t max_lines, uint16_t max_pixel_width);
uint8_t PointInRect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh);
void SafeStringCopy(char *dst, const char *src, uint16_t max_len);
void AppendString(char *dst, const char *src, uint16_t max_len);
char *FindFieldValue(char *buf, const char *field);
void USART1_SendString(const char *str);
uint8_t USART1_ReadLine(char *buf, uint16_t max_len, uint32_t timeout);
void USART1_ClearRxBuffer(void);
void Delay(__IO uint32_t nCount);

#endif
