#ifndef __BSP_LED_RGB_H
#define __BSP_LED_RGB_H

#include "stm32f10x.h"

// 定义最大亮度分辨率
#define RGB_MAX_BRIGHTNESS  255

// 函数声明
void LED_RGB_Init(void);
void LED_RGB_SetColor(uint8_t r, uint8_t g, uint8_t b);
void LED_RGB_Off(void);

// 呼吸灯状态逻辑函数 (非阻塞)
void LED_RGB_BreathingHandler(uint8_t is_playing);

#endif /* __BSP_LED_RGB_H */