#ifndef __BSP_LED_RGB_H
#define __BSP_LED_RGB_H

#include "stm32f10x.h"

#define RGB_MAX_BRIGHTNESS  255

void LED_RGB_Init(void);
void LED_RGB_SetColor(uint8_t r, uint8_t g, uint8_t b);
void LED_RGB_Off(void);

void LED_RGB_BreathingHandler(uint8_t is_playing);

#endif /* __BSP_LED_RGB_H */