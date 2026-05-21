#include "key_input.h"

/*
 * 官方按键定义：
 * K1 -> PA0
 * K2 -> PC13
 * 按下为高电平
 */

#define K1_GPIO_CLK      RCC_APB2Periph_GPIOA
#define K1_GPIO_PORT     GPIOA
#define K1_GPIO_PIN      GPIO_Pin_0

#define K2_GPIO_CLK      RCC_APB2Periph_GPIOC
#define K2_GPIO_PORT     GPIOC
#define K2_GPIO_PIN      GPIO_Pin_13

#define KEY_PRESSED      1
#define KEY_RELEASED     0

/*
 * 因为没有使用精确定时器，这里的计数依赖主循环调用频率。
 * 如果长按太难触发，就调小 KEY_LONG_COUNT。
 * 如果太容易触发，就调大。
 */
#define KEY_DEBOUNCE_COUNT  200
#define KEY_LONG_COUNT      50000

typedef struct
{
    uint8_t last_level;
    uint8_t stable_level;
    uint16_t debounce_count;
    uint16_t press_count;
    uint8_t long_reported;
} KeyState;

static KeyState k1_state;
static KeyState k2_state;

static uint8_t Key_Read(GPIO_TypeDef *port, uint16_t pin);
static KeyEvent Key_UpdateOne(KeyState *state, uint8_t level, KeyEvent short_event, KeyEvent long_event);


void KeyInput_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(K1_GPIO_CLK | K2_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = K1_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(K1_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = K2_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(K2_GPIO_PORT, &GPIO_InitStructure);

    k1_state.last_level = KEY_RELEASED;
    k1_state.stable_level = KEY_RELEASED;
    k1_state.debounce_count = 0;
    k1_state.press_count = 0;
    k1_state.long_reported = 0;

    k2_state.last_level = KEY_RELEASED;
    k2_state.stable_level = KEY_RELEASED;
    k2_state.debounce_count = 0;
    k2_state.press_count = 0;
    k2_state.long_reported = 0;
}


KeyEvent KeyInput_Update(void)
{
    KeyEvent event;

    event = Key_UpdateOne(
        &k1_state,
        Key_Read(K1_GPIO_PORT, K1_GPIO_PIN),
        KEY_EVENT_K1_SHORT,
        KEY_EVENT_K1_LONG
    );

    if (event != KEY_EVENT_NONE)
    {
        return event;
    }

    event = Key_UpdateOne(
        &k2_state,
        Key_Read(K2_GPIO_PORT, K2_GPIO_PIN),
        KEY_EVENT_K2_SHORT,
        KEY_EVENT_K2_LONG
    );

    return event;
}


static uint8_t Key_Read(GPIO_TypeDef *port, uint16_t pin)
{
    if (GPIO_ReadInputDataBit(port, pin) == Bit_SET)
    {
        return KEY_PRESSED;
    }

    return KEY_RELEASED;
}


static KeyEvent Key_UpdateOne(KeyState *state, uint8_t level, KeyEvent short_event, KeyEvent long_event)
{
    /*
     * 简单消抖：连续多次读到同一个电平，才认为稳定
     */
    if (level != state->last_level)
    {
        state->last_level = level;
        state->debounce_count = 0;
        return KEY_EVENT_NONE;
    }

    if (state->debounce_count < KEY_DEBOUNCE_COUNT)
    {
        state->debounce_count++;
        return KEY_EVENT_NONE;
    }

    /*
     * 稳定电平发生变化
     */
    if (level != state->stable_level)
    {
        state->stable_level = level;

        /*
         * 刚按下：开始计数
         */
        if (state->stable_level == KEY_PRESSED)
        {
            state->press_count = 0;
            state->long_reported = 0;
            return KEY_EVENT_NONE;
        }

        /*
         * 刚松开：根据按下持续时间判断短按/长按
         * 注意：长按事件也在松手后触发，避免一按下就立刻执行
         */
        if (state->stable_level == KEY_RELEASED)
        {
            KeyEvent event;

            if (state->press_count >= KEY_LONG_COUNT)
            {
                event = long_event;
            }
            else if (state->press_count > 0)
            {
                event = short_event;
            }
            else
            {
                event = KEY_EVENT_NONE;
            }

            state->press_count = 0;
            state->long_reported = 0;

            return event;
        }
    }

    /*
     * 稳定按下期间只计数，不立刻触发长按事件
     */
    if (state->stable_level == KEY_PRESSED)
    {
        if (state->press_count < 0xFFFF)
        {
            state->press_count++;
        }
    }

    return KEY_EVENT_NONE;
}