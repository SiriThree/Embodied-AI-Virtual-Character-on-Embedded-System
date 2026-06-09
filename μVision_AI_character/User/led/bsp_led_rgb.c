#include "bsp_led_rgb.h"
#include "../emotion.h"
#include "../ai_app_data.h"

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


void LED_RGB_BreathingHandler(uint8_t is_playing)
{
    // --- 参数配置 ---
    #define BREATH_SPEED    (g_led_speed)
    #define PEAK_VAL       255   
    #define CYCLE_STEPS    (PEAK_VAL * 2) 

    static uint32_t speed_counter = 0;
    static uint16_t local_tick = 0; 
    static uint8_t  force_running = 0; 
    
    uint16_t linear_val;
    uint32_t brightness_factor; 
    
    uint8_t base_r = 0, base_g = 0, base_b = 0;
    uint8_t out_r;
    uint8_t out_g;
    uint8_t out_b;

    if (is_playing) force_running = 1;
    if (force_running == 0) {
        LED_RGB_Off();
        return;
    }

    speed_counter++;
    if (speed_counter < BREATH_SPEED) return;
    speed_counter = 0;

    switch (Emotion_GetFace()) {
        case FACE_HAPPY:     base_r = 255; base_g = 200; base_b = 0;   break;
        case FACE_CONTENT:   base_r = 50;  base_g = 255; base_b = 50;  break;
        case FACE_RELAXED:   base_r = 0;   base_g = 200; base_b = 200; break;
        case FACE_SURPRISED: base_r = 255; base_g = 255; base_b = 255; break;
        case FACE_NEUTRAL:   base_r = 100; base_g = 100; base_b = 100; break;
        case FACE_BORED:     base_r = 60;  base_g = 60;  base_b = 80;  break;
        case FACE_ANGRY:     base_r = 255; base_g = 0;   base_b = 0;   break;
        case FACE_SAD:       base_r = 0;   base_g = 0;   base_b = 255; break;
        case FACE_DEPRESSED: base_r = 40;  base_g = 0;   base_b = 80;  break;
        case FACE_THINKING:  base_r = 255; base_g = 150; base_b = 0;   break;
        default:             base_r = 0;   base_g = 0;   base_b = 0;   break;
    }

    if (local_tick < PEAK_VAL) {
        linear_val = local_tick;           
    } else {
        linear_val = CYCLE_STEPS - local_tick; 
    }

    brightness_factor = (uint32_t)linear_val * linear_val * RGB_MAX_BRIGHTNESS / 65025;

    out_r = (uint16_t)(base_r * brightness_factor / RGB_MAX_BRIGHTNESS);
    out_g = (uint16_t)(base_g * brightness_factor / RGB_MAX_BRIGHTNESS);
    out_b = (uint16_t)(base_b * brightness_factor / RGB_MAX_BRIGHTNESS);

    LED_RGB_SetColor(out_r, out_g, out_b);

    local_tick++;
    if (local_tick >= CYCLE_STEPS) {
        local_tick = 0;
        if (!is_playing) {
            force_running = 0; 
        }
    }
}