#include "stm32f10x.h"

#include "./usart/bsp_usart.h"
#include "./lcd/bsp_ili9341_lcd.h"
#include "./flash/bsp_spi_flash.h"

#include "bsp_xpt2046_lcd.h"
#include "gesture.h"
#include "ai_avatars.h"
#include "key_input.h"


#define LCD_CMD   (*((volatile uint16_t *)FSMC_Addr_ILI9341_CMD))
#define LCD_DATA  (*((volatile uint16_t *)FSMC_Addr_ILI9341_DATA))
#define AVATAR_X   20
#define AVATAR_Y   5
#define IDLE_FEEDBACK_COUNT    800000UL
/* ============================================================
 *  数据结构与全局状态
 * ============================================================ */

typedef struct
{
    const char *labelText;   // 底部选择栏显示，短
    const char *showText;    // 对话区显示，完整
    const char *sendText;    // 发给 ESP32 / LLM
    AvatarState avatar;
} PresetQuestion;


static PresetQuestion g_questions[] =
{
    {"打招呼", "你：你好呀，今天也见到你了", 
     "Hello, nice to see you again today.\n", AVATAR_HAPPY},

    {"你是谁", "你：你是谁呀，可以介绍一下自己吗", 
     "Who are you? Can you introduce yourself briefly?\n", AVATAR_CURIOUS},

    {"陪陪我", "你：今天有点累，陪我一会儿吧", 
     "I feel a little tired today. Please stay with me for a while.\n", AVATAR_GENTLE},

    {"没动力", "你：我现在有点没动力", 
     "I feel a little unmotivated right now.\n", AVATAR_TIRED},

    {"鼓励我", "你：给我一点鼓励好不好", 
     "Please encourage me a little.\n", AVATAR_HAPPY},

    {"讲笑话", "你：讲个轻松一点的笑话吧", 
     "Tell me a light and simple joke.\n", AVATAR_HAPPY},

    {"聊天", "你：我想和你随便聊聊天", 
     "I want to casually chat with you.\n", AVATAR_GENTLE},

    {"安静陪伴", "你：我想安静一会儿，你陪着就好", 
     "I want to be quiet for a while. Just stay with me.\n", AVATAR_GENTLE},

    {"有点紧张", "你：我有点紧张，帮我放松一下", 
     "I feel a little nervous. Help me relax.\n", AVATAR_GENTLE},

    {"不想动", "你：今天好像什么都不想做", 
     "I don't feel like doing anything today.\n", AVATAR_TIRED},

    {"会陪我吗", "你：你会一直陪着我吗", 
     "Will you always stay with me?\n", AVATAR_SHY},

    {"夸夸我", "你：夸夸我吧，我想听点好的", 
     "Please praise me a little. I want to hear something nice.\n", AVATAR_HAPPY},

    {"完成一点事", "你：我刚刚完成了一点事情", 
     "I just finished something, even if it was small.\n", AVATAR_HAPPY},

    {"想逃避", "你：我有点想逃避现实", 
     "I kind of want to escape from reality for a while.\n", AVATAR_TIRED},

    {"休息模式", "你：陪我进入休息模式吧", 
     "Please enter quiet rest mode with me.\n", AVATAR_GENTLE},

    {"温柔一句", "你：对我说一句温柔的话", 
     "Say something gentle to me.\n", AVATAR_SHY}
};

#define QUESTION_COUNT  (sizeof(g_questions) / sizeof(g_questions[0]))

static uint8_t g_question_index = 0;

typedef struct
{
    const char *text;
    AvatarState avatar;
} IdleFeedback;

static IdleFeedback g_idle_feedbacks[] =
{
    {"她：嘿，在忙什么？",        AVATAR_CURIOUS},
    {"她：有没有想我呀~",        AVATAR_SHY},
    {"她：我刚刚在等你哦。",      AVATAR_GENTLE},
    {"她：嘿，你怎么不说话啦。",  AVATAR_CURIOUS},
    {"她：偷偷发呆中？",          AVATAR_HAPPY},
    {"她：我可没有睡着哦。",      AVATAR_HAPPY},
    {"她：再不理我我要闹啦~",     AVATAR_SHY},
    {"她：今天也辛苦你啦。",      AVATAR_GENTLE},
    {"她：来，给你一点元气。",    AVATAR_HAPPY},
    {"她：我在这里陪着你。",      AVATAR_GENTLE}
};

#define IDLE_FEEDBACK_NUM  (sizeof(g_idle_feedbacks) / sizeof(g_idle_feedbacks[0]))

static uint32_t g_idle_count = 0;
static uint8_t g_idle_feedback_index = 0;
static uint8_t g_idle_showing = 0;

/* ============================================================
 *  函数声明
 * ============================================================ */

/* 基础 */
static void Delay(__IO uint32_t nCount);
static void System_Init_All(void);
static void App_MainLoop(void);

/* LCD UI */
static void AI_UI_Init(void);
static void AI_UI_DrawFrame(void);
static void AI_UI_SetAvatar(AvatarState state);
static void AI_UI_ShowUserText(const char *text);
static void AI_UI_ShowAIText(const char *text);
static void AI_UI_ShowAIText_MultiLine(const char *text);
static void AI_UI_ShowCurrentQuestion(void);
static void AI_UI_ClearDialogArea(void);

/* 串口 */
static void USART1_SendString(const char *str);
static uint8_t USART1_ReadLine(char *buf, uint16_t max_len, uint32_t timeout);
static void USART1_ClearRxBuffer(void);

/* 应用逻辑 */
static void AI_SendPresetQuestion(const char *showText, const char *sendText, AvatarState finalAvatar);
static void AI_Question_Next(void);
static void AI_Question_Prev(void);
static void AI_Question_SendCurrent(void);
static void AI_EnterRestMode(void);
static void AI_HandleKeyEvent(KeyEvent key);
static void AI_HandleGestureEvent(GestureType gesture);



/* 文本换行 */
static const char* CopyLineByPixelWidth(const char *src, char *dst, uint16_t max_pixel_width);


static void AI_Idle_Update(void);
static void AI_Idle_Reset(void);
static void AI_Idle_Feedback(void);

/* ============================================================
 *  主函数
 * ============================================================ */

int main(void)
{
    System_Init_All();
    App_MainLoop();

    while (1)
    {
    }
}


/* ============================================================
 *  系统初始化
 * ============================================================ */

static void System_Init_All(void)
{
    /* LCD 初始化 */
    ILI9341_Init();

    /* USART1 初始化：PA9 TX, PA10 RX，与 ESP32 通信 */
    USART_Config();

    /* LCD 扫描方向，和触摸校准方向保持一致 */
    ILI9341_GramScan(6);

    /* 触摸屏初始化与校准参数读取 */
    XPT2046_Init();
    Calibrate_or_Get_TouchParaWithFlash(6, 0);

    /* 手势识别与按键识别 */
    Gesture_Init();
    KeyInput_Init();

    /* AI UI 初始化 */
    AI_UI_Init();

    /*
     * 清掉 ESP32 启动时可能发来的 ESP32 BOOT OK，
     * 避免 STM32 把启动信息当成 AI 回复。
     */
    Delay(0x7FFFFF);
    USART1_ClearRxBuffer();
}


/* ============================================================
 *  主循环：同时处理 K1/K2 和触摸手势
 * ============================================================ */

static void App_MainLoop(void)
{
    while (1)
    {
        KeyEvent key;
        GestureType gesture;
        uint8_t has_input = 0;

        key = KeyInput_Update();
        gesture = Gesture_Update();

        /*
         * 只要检测到按键事件，就认为用户有操作
         */
        if (key != KEY_EVENT_NONE)
        {
            has_input = 1;
            AI_Idle_Reset();
        }

        /*
         * 只要检测到手势事件，也认为用户有操作
         * 包括 GESTURE_TAP，虽然我们不触发请求，但它仍然代表用户碰了屏幕
         */
        if (gesture != GESTURE_NONE)
        {
            has_input = 1;
            AI_Idle_Reset();
        }

        /*
         * 处理按键事件
         */
        AI_HandleKeyEvent(key);

        /*
         * 处理手势事件
         */
        AI_HandleGestureEvent(gesture);

        /*
         * 没有任何输入时，才更新待机主动反馈系统
         */
        if (!has_input)
        {
            AI_Idle_Update();
        }
    }
}


static void AI_HandleKeyEvent(KeyEvent key)
{
    switch (key)
    {
        case KEY_EVENT_K1_SHORT:
            AI_Question_Next();
            break;

        case KEY_EVENT_K1_LONG:
            AI_Question_Prev();
            break;

        case KEY_EVENT_K2_SHORT:
            AI_Question_SendCurrent();
            break;

        case KEY_EVENT_K2_LONG:
            AI_EnterRestMode();
            break;

        default:
            break;
    }
}


static void AI_HandleGestureEvent(GestureType gesture)
{
    switch (gesture)
{
    case GESTURE_SWIPE_RIGHT:
        /*
         * 右滑：靠近 / 唤醒她
         */
        AI_SendPresetQuestion(
            "你：轻轻靠近了她",
            "I gently come closer to you.\n",
            AVATAR_CURIOUS
        );
        break;

    case GESTURE_SWIPE_LEFT:
        /*
         * 左滑：请求陪伴
         */
        AI_SendPresetQuestion(
            "你：想让她陪你一会儿",
            "I want you to stay with me for a while.\n",
            AVATAR_GENTLE
        );
        break;

    case GESTURE_SWIPE_UP:
        /*
         * 上滑：让她开心一点
         */
        AI_SendPresetQuestion(
            "你：想让她开心一点",
            "I want to make you a little happier.\n",
            AVATAR_HAPPY
        );
        break;

    case GESTURE_SWIPE_DOWN:
        /*
         * 下滑：表达疲惫
         */
        AI_SendPresetQuestion(
            "你：今天有点累了",
            "I feel a little tired today.\n",
            AVATAR_TIRED
        );
        break;

    case GESTURE_LONG_PRESS:
        /*
         * 长按：安静陪伴
         */
        AI_SendPresetQuestion(
            "你：安静地陪着她",
            "I want to stay quietly with you.\n",
            AVATAR_GENTLE
        );
        break;

    case GESTURE_TAP:
        /*
         * 点击不触发，避免误触
         */
        break;

    default:
        break;
}
}


/* ============================================================
 *  问题池逻辑：K1/K2 控制
 * ============================================================ */

static void AI_Question_Next(void)
{
    g_question_index++;

    if (g_question_index >= QUESTION_COUNT)
    {
        g_question_index = 0;
    }

    AI_UI_SetAvatar(AVATAR_NORMAL_IDLE);
    AI_UI_ShowCurrentQuestion();
}


static void AI_Question_Prev(void)
{
    if (g_question_index == 0)
    {
        g_question_index = QUESTION_COUNT - 1;
    }
    else
    {
        g_question_index--;
    }

    AI_UI_SetAvatar(AVATAR_NORMAL_IDLE);
    AI_UI_ShowCurrentQuestion();
}


static void AI_Question_SendCurrent(void)
{
    AI_SendPresetQuestion(
        g_questions[g_question_index].showText,
        g_questions[g_question_index].sendText,
        g_questions[g_question_index].avatar
    );
}


static void AI_EnterRestMode(void)
{
    AI_UI_SetAvatar(AVATAR_NORMAL_IDLE);

    AI_UI_ClearDialogArea();

    LCD_SetColors(GREEN, BLACK);
		ILI9341_DispString_EN_CH(18, 230, "AI: 休息中");

    AI_UI_ShowCurrentQuestion();
}


/* ============================================================
 *  AI 请求：显示问题 → thinking 表情 → 等 ESP32 → 显示回复
 * ============================================================ */

static void AI_SendPresetQuestion(const char *showText, const char *sendText, AvatarState finalAvatar)
{
    char esp32_buf[128];

    AI_UI_ShowUserText(showText);

    /* 等待回复时切换成思考表情 */
    AI_UI_SetAvatar(AVATAR_THINKING);
    AI_UI_ShowAIText("AI: 正在思考...");

    USART1_ClearRxBuffer();
    USART1_SendString(sendText);

    if (USART1_ReadLine(esp32_buf, sizeof(esp32_buf), 0x2FFFFFF))
    {
        AI_UI_SetAvatar(finalAvatar);
        AI_UI_ShowAIText_MultiLine(esp32_buf);
    }
    else
    {
        AI_UI_SetAvatar(AVATAR_TIRED);
        AI_UI_ShowAIText("AI: ESP32 Timeout");
    }

    AI_UI_ShowCurrentQuestion();
}


/* ============================================================
 *  UI：初始化、框架、头像、文本区域
 * ============================================================ */

static void AI_UI_Init(void)
{
    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);

    AI_UI_DrawFrame();
    AI_UI_SetAvatar(AVATAR_NORMAL_IDLE);

    AI_UI_ShowUserText("User: 你好");
    AI_UI_ShowAIText("AI: 我是你的AI伙伴");

    AI_UI_ShowCurrentQuestion();
}


static void AI_UI_DrawFrame(void)
{
    /*
     * 新布局：
     * 0   ~ 180 ：AI人物图像区域
     * 185 ~ 270 ：对话区域
     * 288 ~ 316 ：当前问题区域
     */

    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);

    /*
     * 头像区域边框
     * 头像本身是 120x160，显示在 x=60, y=10
     * 外框稍微比头像大一点
     */
    LCD_SetTextColor(BLUE);
    ILI9341_DrawRectangle(AVATAR_X - 2, AVATAR_Y - 2,
                      AI_AVATAR_WIDTH + 4,
                      AI_AVATAR_HEIGHT + 4,
                      0);

    /*
     * 对话区域边框
     */
    LCD_SetTextColor(WHITE);
    ILI9341_DrawRectangle(10, 185, 220, 95, 0);

    /*
     * 当前问题区域分割线
     */
    LCD_SetTextColor(CYAN);
    ILI9341_DrawLine(0, 284, LCD_X_LENGTH, 284);
}


static void AI_UI_SetAvatar(AvatarState state)
{
    uint32_t i;
    const uint16_t *img;

    img = AI_Avatar_GetImage(state);

    /*
     * 清除完整头像区域，避免旧头像残留重叠
     */
    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(AVATAR_X, AVATAR_Y, AI_AVATAR_WIDTH, AI_AVATAR_HEIGHT);

    ILI9341_OpenWindow(AVATAR_X, AVATAR_Y, AI_AVATAR_WIDTH, AI_AVATAR_HEIGHT);

    LCD_CMD = CMD_SetPixel;

    for (i = 0; i < AI_AVATAR_WIDTH * AI_AVATAR_HEIGHT; i++)
    {
        LCD_DATA = img[i];
    }
}


static void AI_UI_ClearDialogArea(void)
{
    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(12, 214, 216, 70);
}


static void AI_UI_ShowUserText(const char *text)
{
    char line1[80];
    char line2[80];
    const char *p;

    line1[0] = '\0';
    line2[0] = '\0';

    /*
     * 对话框宽度约 220 像素，
     * x=18 起始，可用宽度约 200 像素。
     */
    p = CopyLineByPixelWidth(text, line1, 200);

    if (*p != '\0')
    {
        CopyLineByPixelWidth(p, line2, 200);
    }

    LCD_SetColors(WHITE, BLACK);

    /* 清除 User 两行区域 */
    ILI9341_Clear(12, 190, 216, 42);

    LCD_SetColors(YELLOW, BLACK);

    ILI9341_DispString_EN_CH(18, 192, line1);

    if (line2[0] != '\0')
    {
        ILI9341_DispString_EN_CH(18, 212, line2);
    }
}

static void AI_UI_ShowAIText(const char *text)
{
    LCD_SetColors(WHITE, BLACK);

    /* 清除 AI 回复区域 */
    ILI9341_Clear(12, 232, 216, 45);

    LCD_SetColors(GREEN, BLACK);
    ILI9341_DispString_EN_CH(18, 236, (char *)text);
}


static void AI_UI_ShowAIText_MultiLine(const char *text)
{
    char line1[80];
    char line2[80];
    const char *p;

    line1[0] = '\0';
    line2[0] = '\0';

    /*
     * 对话框可用宽度约 200 像素。
     * GBK 中文约 16 像素，英文约 8 像素。
     */
    p = CopyLineByPixelWidth(text, line1, 200);

    if (*p != '\0')
    {
        CopyLineByPixelWidth(p, line2, 200);
    }

    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(12, 232, 216, 45);

		LCD_SetColors(GREEN, BLACK);

		ILI9341_DispString_EN_CH(18, 236, line1);

		if (line2[0] != '\0')
		{
    ILI9341_DispString_EN_CH(18, 256, line2);
		}
}


static void AI_UI_ShowCurrentQuestion(void)
{
    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(5, 288, 230, 28);

    LCD_SetColors(CYAN, BLACK);
    ILI9341_DispString_EN_CH(8, 292, "说:");

    LCD_SetColors(YELLOW, BLACK);
    ILI9341_DispString_EN_CH(45, 292, (char *)g_questions[g_question_index].labelText);
}


static void AI_Idle_Reset(void)
{
    g_idle_count = 0;
    g_idle_showing = 0;
}


static void AI_Idle_Feedback(void)
{
    IdleFeedback *item;

    item = &g_idle_feedbacks[g_idle_feedback_index];

    AI_UI_SetAvatar(item->avatar);
    AI_UI_ShowAIText_MultiLine(item->text);

    g_idle_feedback_index++;

    if (g_idle_feedback_index >= IDLE_FEEDBACK_NUM)
    {
        g_idle_feedback_index = 0;
    }

    g_idle_showing = 1;
}


static void AI_Idle_Update(void)
{
    /*
     * 不管当前是不是正在显示待机反馈，
     * 都继续累计无人操作时间。
     * 这样她可以隔一段时间继续主动说下一句话。
     */
    g_idle_count++;

    if (g_idle_count >= IDLE_FEEDBACK_COUNT)
    {
        g_idle_count = 0;
        AI_Idle_Feedback();
    }
}

/* ============================================================
 *  文本分行：按像素宽度切分，避免半个 GBK 汉字
 * ============================================================ */

static const char* CopyLineByPixelWidth(const char *src, char *dst, uint16_t max_pixel_width)
{
    uint16_t pixel_width = 0;
    uint16_t dst_i = 0;

    while (*src != '\0')
    {
        uint8_t c = (uint8_t)(*src);

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


/* ============================================================
 *  USART1：STM32 <-> ESP32
 * ============================================================ */

static void USART1_SendString(const char *str)
{
    while (*str)
    {
        USART_SendData(USART1, (uint8_t)(*str));

        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        {
        }

        str++;
    }
}


static uint8_t USART1_ReadLine(char *buf, uint16_t max_len, uint32_t timeout)
{
    uint16_t i = 0;

    while (timeout--)
    {
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            char ch;

            ch = (char)USART_ReceiveData(USART1);

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


static void USART1_ClearRxBuffer(void)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
        USART_ReceiveData(USART1);
    }
}


/* ============================================================
 *  简单延时
 * ============================================================ */

static void Delay(__IO uint32_t nCount)
{
    while (nCount--)
    {
    }
}
