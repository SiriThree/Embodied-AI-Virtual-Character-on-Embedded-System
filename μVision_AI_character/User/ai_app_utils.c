/* Shared helpers: text wrapping, string helpers and UART packet helpers. */
#include "stm32f10x.h"
#include "./usart/bsp_usart.h"

#include <string.h>

#include "ai_app_utils.h"

const char *CopyLineByPixelWidth(const char *src, char *dst, uint16_t max_pixel_width)
{
    uint16_t pixel_width;
    uint16_t dst_i;

    pixel_width = 0;
    dst_i = 0;

    while (*src != '\0')
    {
        uint8_t c;

        c = (uint8_t)(*src);

        if (c >= 0x80)
        {
            if (src[1] == '\0')
            {
                break;
            }

            if (pixel_width + 16 > max_pixel_width)
            {
                break;
            }

            dst[dst_i++] = src[0];
            dst[dst_i++] = src[1];
            src += 2;
            pixel_width += 16;
        }
        else
        {
            if (pixel_width + 8 > max_pixel_width)
            {
                break;
            }

            dst[dst_i++] = src[0];
            src += 1;
            pixel_width += 8;
        }
    }

    dst[dst_i] = '\0';
    return src;
}

uint16_t TextPixelWidth(const char *text)
{
    uint16_t pixel_width;

    pixel_width = 0;
    while (*text != '\0')
    {
        uint8_t c;

        c = (uint8_t)(*text);
        if (c >= 0x80 && text[1] != '\0')
        {
            pixel_width += 16;
            text += 2;
        }
        else
        {
            pixel_width += 8;
            text += 1;
        }
    }

    return pixel_width;
}

void TrimLineWithEllipsis(char *text, uint16_t max_pixel_width)
{
    static const char ellipsis[] = "...";
    uint16_t ellipsis_width;
    uint16_t len;

    if (text[0] == '\0')
    {
        return;
    }

    if (TextPixelWidth(text) <= max_pixel_width)
    {
        return;
    }

    ellipsis_width = TextPixelWidth(ellipsis);
    len = (uint16_t)strlen(text);

    while (len > 0 && TextPixelWidth(text) + ellipsis_width > max_pixel_width)
    {
        if (((uint8_t)text[len - 1]) >= 0x80 && len >= 2 && ((uint8_t)text[len - 2]) >= 0x80)
        {
            len -= 2;
        }
        else
        {
            len -= 1;
        }

        text[len] = '\0';
    }

    AppendString(text, ellipsis, MAX_TEXT_LEN);
}

uint8_t WrapTextLines(const char *src, char lines[][MAX_TEXT_LEN], uint8_t max_lines, uint16_t max_pixel_width)
{
    uint8_t count;
    const char *p;

    count = 0;
    p = src;

    while (*p != '\0' && count < max_lines)
    {
        p = CopyLineByPixelWidth(p, lines[count], max_pixel_width);

        if (count == (uint8_t)(max_lines - 1) && *p != '\0')
        {
            TrimLineWithEllipsis(lines[count], max_pixel_width);
        }

        count++;
    }

    return count;
}

uint8_t PointInRect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)
{
    if (x < rx || y < ry)
    {
        return 0;
    }

    if (x >= (uint16_t)(rx + rw) || y >= (uint16_t)(ry + rh))
    {
        return 0;
    }

    return 1;
}

void SafeStringCopy(char *dst, const char *src, uint16_t max_len)
{
    uint16_t i;

    if (max_len == 0)
    {
        return;
    }

    for (i = 0; i < (uint16_t)(max_len - 1) && src[i] != '\0'; i++)
    {
        dst[i] = src[i];
    }

    dst[i] = '\0';
}

void AppendString(char *dst, const char *src, uint16_t max_len)
{
    uint16_t len;
    uint16_t i;

    len = (uint16_t)strlen(dst);
    if (len >= max_len - 1)
    {
        return;
    }

    for (i = 0; i < (uint16_t)(max_len - 1 - len) && src[i] != '\0'; i++)
    {
        dst[len + i] = src[i];
    }

    dst[len + i] = '\0';
}

char *FindFieldValue(char *buf, const char *field)
{
    char *pos;
    char *end;

    pos = strstr(buf, field);
    if (pos == 0)
    {
        return 0;
    }

    pos += strlen(field);
    end = strchr(pos, '|');
    if (end != 0)
    {
        *end = '\0';
    }

    return pos;
}

void USART1_SendString(const char *str)
{
    while (*str)
    {
        USART_SendData(DEBUG_USARTx, (uint8_t)(*str));

        while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_TXE) == RESET)
        {
        }

        str++;
    }
}

uint8_t USART1_ReadLine(char *buf, uint16_t max_len, uint32_t timeout)
{
    uint16_t i;

    i = 0;

    while (timeout--)
    {
        if (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_RXNE) != RESET)
        {
            char ch;

            ch = (char)USART_ReceiveData(DEBUG_USARTx);

            if (ch == '\r')
            {
                continue;
            }

            if (ch == '\n')
            {
                buf[i] = '\0';
                return 1;
            }

            if (i < max_len - 1)
            {
                buf[i++] = ch;
            }
        }
    }

    buf[i] = '\0';
    return 0;
}

void USART1_ClearRxBuffer(void)
{
    while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_RXNE) != RESET)
    {
        USART_ReceiveData(DEBUG_USARTx);
    }
}

void Delay(__IO uint32_t nCount)
{
    while (nCount--)
    {
    }
}
