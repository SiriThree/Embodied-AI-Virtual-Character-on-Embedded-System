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
 * @brief  平滑退出版呼吸灯处理函数
 * @param  is_playing: 外部逻辑信号 (1:开始/持续呼吸, 0:停止)
 */
void LED_RGB_BreathingHandler(uint8_t is_playing)
{
    // 参数配置
    #define BREATH_SPEED   1000  // 呼吸速度 (你之前测试的1000比较好)
    #define PEAK_VAL       255   // 呼吸的顶峰值
    #define CYCLE_STEPS    (PEAK_VAL * 2) // 总步数 (0->255->0 为 510 步)

    static uint32_t speed_counter = 0;
    static uint16_t local_tick = 0; 
    static uint8_t  force_running = 0; // 内部状态锁
    uint16_t linear_val;
    uint32_t brightness;

    // 状态机逻辑：
    // 只要外部想让它亮，或者灯还没熄灭到 0，就强制继续运行
    if (is_playing) {
        force_running = 1;
    }

    if (force_running == 0) {
        LED_RGB_Off();
        return;
    }

    // 速度控制
    speed_counter++;
    if (speed_counter < BREATH_SPEED) return;
    speed_counter = 0;

    // 计算线性亮度进度 (0 -> 255 -> 0)
    if (local_tick < PEAK_VAL) {
        linear_val = local_tick;           // 上坡阶段
    } else {
        linear_val = CYCLE_STEPS - local_tick; // 下坡阶段
    }

    // --- 核心优化：平方律映射 (指数级平滑) ---
    // 这个公式保证了亮度在接近 0 的时候变化非常细腻，不会产生“陡峭”感
    // 分母 65025 是 255*255 的结果
    brightness = (uint32_t)linear_val * linear_val * RGB_MAX_BRIGHTNESS / 65025;
    
    LED_RGB_SetColor(0, (uint8_t)brightness, (uint8_t)brightness);

    // 更新进度
    local_tick++;

    // 检查是否走完了一个完整的周期 (0 -> Max -> 0)
    if (local_tick >= CYCLE_STEPS) {
        local_tick = 0;
        
        // 关键点：当一个完整周期结束时，检查外部是否还要求继续呼吸
        if (!is_playing) {
            force_running = 0; // 只有在终点且外部不要求播放时，才真正关闭
        }
    }
}