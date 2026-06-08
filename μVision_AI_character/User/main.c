#include "stm32f10x.h"

#include "./usart/bsp_usart.h"
#include "./lcd/bsp_ili9341_lcd.h"

#include "bsp_xpt2046_lcd.h"
#include "gesture.h"
#include "key_input.h"
#include "ai_app_utils.h"
#include "ai_app.h"
#include "./led/bsp_led_rgb.h"

static void System_Init_All(void);

int main(void)
{
    System_Init_All();
    AI_App_Run();

    while (1)
    {
    }
}

static void System_Init_All(void)
{
    ILI9341_Init();
    USART_Config();
    ILI9341_GramScan(6);

    XPT2046_Init();
    Calibrate_or_Get_TouchParaWithFlash(6, 0);
    LED_RGB_Init();

    Gesture_Init();
    KeyInput_Init();

    AI_App_Init();

    Delay(0x7FFFFF);
    USART1_ClearRxBuffer();
}
