#include "bsp_led_rgb.h"

/**
 * @brief  初始化 TIM3 PWM 模式用于驱动 RGB LED
 * @note   PB5->TIM3_CH2(重映射), PB0->TIM3_CH3, PB1->TIM3_CH4
 */
void LED_RGB_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    // 1. 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 2. PB5 重映射到 TIM3_CH2 (部分重映射)
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); 

    // 3. 配置引脚为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 4. 定时器基础配置: 1kHz 频率
    TIM_TimeBaseStructure.TIM_Period = RGB_MAX_BRIGHTNESS; 
    TIM_TimeBaseStructure.TIM_Prescaler = 71; // 72MHz/(71+1) = 1MHz计数频率
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // 5. PWM 模式配置 (PWM1模式 + 极性低 = 占空比越大越亮，适配共阳极)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; 

    TIM_OC2Init(TIM3, &TIM_OCInitStructure); // R
    TIM_OC3Init(TIM3, &TIM_OCInitStructure); // G
    TIM_OC4Init(TIM3, &TIM_OCInitStructure); // B

    // 6. 使能定时器
    TIM_Cmd(TIM3, ENABLE);
    
    LED_RGB_Off(); // 初始熄灭
}

void LED_RGB_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    TIM_SetCompare2(TIM3, r);
    TIM_SetCompare3(TIM3, g);
    TIM_SetCompare4(TIM3, b);
}

void LED_RGB_Off(void)
{
    LED_RGB_SetColor(0, 0, 0);
}

/**
 * @brief  呼吸灯处理函数，建议 10ms-20ms 调用一次
 * @param  is_playing: 1开启呼吸, 0熄灭
 */
void LED_RGB_BreathingHandler(uint8_t is_playing)
{
    static uint8_t brightness = 0;
    static int8_t step = 1;

    if (!is_playing) {
        LED_RGB_Off();
        brightness = 0;
        return;
    }

    // 呼吸算法
    brightness += step;
    if (brightness >= 200 || brightness <= 5) {
        step = -step;
    }

    // 默认呼吸颜色：青色 (G+B)，你可以根据心情变量在这里修改颜色
    LED_RGB_SetColor(0, brightness, brightness);
}