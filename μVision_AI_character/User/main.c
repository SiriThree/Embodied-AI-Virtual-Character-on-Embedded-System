#include "stm32f10x.h"



#include "./usart/bsp_usart.h"

#include "./lcd/bsp_ili9341_lcd.h"

#include "./flash/bsp_spi_flash.h"



#include "bsp_xpt2046_lcd.h"

#include "gesture.h"

#include "ai_avatars.h"

#include "key_input.h"



#include <string.h>



#define LCD_CMD   (*((volatile uint16_t *)FSMC_Addr_ILI9341_CMD))

#define LCD_DATA  (*((volatile uint16_t *)FSMC_Addr_ILI9341_DATA))



#define AVATAR_X                 32

#define AVATAR_Y                 24

#define AVATAR_DISP_W            176

#define AVATAR_DISP_H            132

#define UI_TEXT_WIDTH            192
#define MAX_SCENE_COUNT          12

#define MAX_OPTION_COUNT         3

#define MAX_TEXT_LEN             80

#define MAX_PROTOCOL_BUF         512

#define SCENE_REPLY_TIMEOUT      0x7FFFFFFFUL
#define CHAT_REPLY_TIMEOUT       0x7FFFFFFFUL

#define IDLE_FEEDBACK_COUNT      800000UL



#define COLOR_BG                 BLACK

#define COLOR_PANEL              WHITE

#define COLOR_FRAME              CYAN

#define COLOR_TITLE              MAGENTA

#define COLOR_HIGHLIGHT          YELLOW

#define COLOR_AI_TEXT            WHITE
#define COLOR_USER_TEXT          YELLOW

#define COLOR_HINT               CYAN



#define COVER_BTN_X              50

#define COVER_BTN_Y              248

#define COVER_BTN_W              140

#define COVER_BTN_H              38



#define SCENE_LIST_X             10

#define SCENE_LIST_Y             60

#define SCENE_ITEM_W             220

#define SCENE_ITEM_H             28

#define SCENE_VISIBLE_COUNT      7



#define CHAT_DIALOG_X            6
#define CHAT_DIALOG_Y            162
#define CHAT_DIALOG_W            228
#define CHAT_DIALOG_H            80


#define CHAT_OPTION_X            6
#define CHAT_OPTION_Y            248

#define CHAT_OPTION_W            228
#define CHAT_OPTION_H            23
#define CHAT_TEXT_X_OFFSET       8
#define CHAT_USER_Y_OFFSET       8
#define CHAT_AI_Y_OFFSET         44
#define CHAT_LINE_STEP           18

#define TXT_SCENE_SHOPPING       "\xB9\xE4\xBD\xD6\xD4\xBC\xBB\xE1"
#define TXT_SCENE_SHOPPING_INTRO "\xD2\xBB\xC6\xF0\xC2\xFD\xC2\xFD\xB9\xE4\xBD\xD6\xA3\xAC\xBF\xB4\xBF\xB4\xBD\xF1\xCC\xEC\xCF\xEB\xC2\xF2\xCA\xB2\xC3\xB4\xA1\xA3"
#define TXT_SCENE_GAMING         "\xD2\xBB\xC6\xF0\xBF\xAA\xBA\xDA"
#define TXT_SCENE_GAMING_INTRO   "\xD7\xBC\xB1\xB8\xD2\xBB\xC6\xF0\xB4\xF2\xD3\xCE\xCF\xB7\xA3\xAC\xC6\xF8\xB7\xD5\xBB\xE1\xBA\xDC\xC8\xC8\xC4\xD6\xA1\xA3"
#define TXT_SCENE_CAFE           "\xBF\xA7\xB7\xC8\xB9\xDD"
#define TXT_SCENE_CAFE_INTRO     "\xD7\xF8\xD4\xDA\xB4\xB0\xB1\xDF\xC1\xC4\xCC\xEC\xA3\xAC\xBD\xDA\xD7\xE0\xB0\xB2\xBE\xB2\xD3\xD6\xB7\xC5\xCB\xC9\xA1\xA3"
#define TXT_SCENE_LIBRARY        "\xCD\xBC\xCA\xE9\xB9\xDD"
#define TXT_SCENE_LIBRARY_INTRO  "\xC7\xE1\xC9\xF9\xBD\xBB\xC1\xF7\xA3\xAC\xCF\xF1\xB1\xCB\xB4\xCB\xC5\xE3\xB0\xE9\xD1\xA7\xCF\xB0\xA1\xA3"
#define TXT_SCENE_NIGHT          "\xCD\xED\xB0\xB2\xD2\xB9\xC1\xC4"
#define TXT_SCENE_NIGHT_INTRO    "\xD2\xB9\xC0\xEF\xCB\xB5\xD0\xA9\xD0\xC4\xCA\xC2\xA3\xAC\xCA\xCA\xBA\xCF\xCE\xC2\xC8\xE1\xBB\xA5\xB6\xAF\xA1\xA3"
#define TXT_SCENE_ENCOURAGE      "\xB9\xC4\xC0\xF8\xC4\xA3\xCA\xBD"
#define TXT_SCENE_ENCOURAGE_INTRO "\xCB\xFD\xBB\xE1\xB8\xFC\xCF\xF1\xD6\xCE\xD3\xFA\xCF\xB5\xBB\xEF\xB0\xE9\xA3\xAC\xB6\xE0\xB8\xF8\xC4\xE3\xB9\xC4\xC0\xF8\xA1\xA3"
#define TXT_SCENE_WALK           "\xC9\xA2\xB2\xBD\xB4\xB5\xB7\xE7"
#define TXT_SCENE_WALK_INTRO     "\xD2\xBB\xC6\xF0\xD4\xDA\xB0\xF8\xCD\xED\xC9\xA2\xB2\xBD\xA3\xAC\xC1\xC4\xD0\xA9\xC7\xE1\xCB\xC9\xB5\xC4\xBB\xB0\xCC\xE2\xA1\xA3"
#define TXT_SCENE_DINNER         "\xD2\xBB\xC6\xF0\xB3\xD4\xB7\xB9"
#define TXT_SCENE_DINNER_INTRO   "\xB1\xDF\xB3\xD4\xB1\xDF\xC1\xC4\xA3\xAC\xCA\xCA\xBA\xCF\xC8\xD5\xB3\xA3\xC9\xFA\xBB\xEE\xB8\xD0\xBB\xA5\xB6\xAF\xA1\xA3"
#define TXT_SCENE_MOVIE          "\xB5\xE7\xD3\xB0\xCA\xB1\xBC\xE4"
#define TXT_SCENE_MOVIE_INTRO    "\xB7\xD6\xCF\xED\xB8\xD5\xBF\xB4\xB5\xC4\xBE\xE7\xC7\xE9\xBA\xCD\xB8\xD0\xCA\xDC\xA1\xA3"
#define TXT_SCENE_MUSIC          "\xD2\xF4\xC0\xD6\xB7\xD6\xCF\xED"
#define TXT_SCENE_MUSIC_INTRO    "\xC1\xC4\xB8\xE8\xB5\xA5\xA1\xA2\xD0\xFD\xC2\xC9\xBA\xCD\xBD\xF1\xCC\xEC\xB5\xC4\xD0\xC4\xC7\xE9\xA1\xA3"
#define TXT_SCENE_EXAM           "\xBF\xBC\xCA\xD4\xC7\xB0\xCF\xA6"
#define TXT_SCENE_EXAM_INTRO     "\xCB\xFD\xBB\xE1\xC5\xE3\xC4\xE3\xBB\xBA\xBD\xE2\xBD\xF4\xD5\xC5\xB8\xD0\xA1\xA3"
#define TXT_SCENE_REST           "\xD0\xDD\xCF\xA2\xC5\xE3\xB0\xE9"
#define TXT_SCENE_REST_INTRO     "\xB2\xBB\xD0\xE8\xD2\xAA\xBA\xDC\xB6\xE0\xBB\xB0\xA3\xAC\xCB\xFD\xB0\xB2\xBE\xB2\xC5\xE3\xD7\xC5\xC4\xE3\xA1\xA3"

#define TXT_IDLE_1               "\x41\x49\xA3\xBA\xCE\xD2\xBB\xB9\xD4\xDA\xD5\xE2\xC0\xEF\xB5\xC8\xC4\xE3\xD1\xBD\xA1\xA3"
#define TXT_IDLE_2               "\x41\x49\xA3\xBA\xB5\xE3\xD2\xBB\xCF\xC2\xBF\xAA\xCA\xBC\xA3\xAC\xCE\xD2\xC3\xC7\xC8\xA5\xD0\xC2\xB5\xC4\xB3\xA1\xBE\xB0\xCD\xE6\xB0\xC9\xA1\xA3"
#define TXT_IDLE_3               "\x41\x49\xA3\xBA\xCF\xEB\xC1\xC4\xCC\xEC\xB5\xC4\xBB\xB0\xA3\xAC\xCE\xD2\xCB\xE6\xCA\xB1\xB6\xBC\xD4\xDA\xA1\xA3"
#define TXT_IDLE_4               "\x41\x49\xA3\xBA\xBD\xF1\xCC\xEC\xCF\xEB\xBA\xCD\xCE\xD2\xC8\xA5\xC4\xC4\xD2\xBB\xB8\xF6\xB3\xA1\xBE\xB0\xC4\xD8\xA3\xBF"

#define TXT_CHAT_INIT            "\x41\x49\xA3\xBA\xBD\xF1\xCC\xEC\xCF\xEB\xD4\xF5\xC3\xB4\xBA\xCD\xCE\xD2\xBB\xA5\xB6\xAF\xC4\xD8\xA3\xBF"
#define TXT_OPT_INIT_1           "\xCF\xC8\xC1\xC4\xC1\xC4\xCC\xEC"
#define TXT_OPT_INIT_2           "\xD7\xF6\xB5\xE3\xB3\xA1\xBE\xB0\xBB\xA5\xB6\xAF"
#define TXT_OPT_INIT_3           "\xCC\xFD\xCB\xFD\xD6\xF7\xB6\xAF\xBF\xAA\xBF\xDA"
#define TXT_LOADING              "\x41\x49\xA3\xBA\xB3\xA1\xBE\xB0\xBC\xD3\xD4\xD8\xD6\xD0\x2E\x2E\x2E"
#define TXT_CHAT_SCENE_OK        "\x41\x49\xA3\xBA\xBA\xC3\xD1\xBD\xA3\xAC\xCE\xD2\xC3\xC7\xBD\xF8\xC8\xEB\xD5\xE2\xB8\xF6\xB3\xA1\xBE\xB0\xC2\xFD\xC2\xFD\xC1\xC4\xA1\xA3"
#define TXT_OPT_SCENE_1          "\xC4\xE3\xCF\xC8\xCB\xB5\xCB\xB5\xBD\xF1\xCC\xEC\xCF\xEB\xD7\xF6\xCA\xB2\xC3\xB4"
#define TXT_OPT_SCENE_2          "\xCE\xD2\xC3\xC7\xC0\xB4\xB5\xE3\xD3\xD0\xB4\xFA\xC8\xEB\xB8\xD0\xB5\xC4\xBB\xA5\xB6\xAF"
#define TXT_OPT_SCENE_3          "\xC4\xE3\xCF\xC8\xD6\xF7\xB6\xAF\xBA\xCD\xCE\xD2\xB4\xF2\xD5\xD0\xBA\xF4"
#define TXT_YOU_PREFIX           "\xC4\xE3\xA3\xBA"
#define TXT_THINKING             "\x41\x49\xA3\xBA\xD5\xFD\xD4\xDA\xCB\xBC\xBF\xBC\x2E\x2E\x2E"

#define TXT_SHOP_REPLY           "\xC4\xC7\xCE\xD2\xC3\xC7\xCF\xC8\xC2\xFD\xC2\xFD\xB9\xE4\xA3\xAC\xCE\xD2\xD2\xB2\xCF\xEB\xCC\xFD\xC4\xE3\xCC\xF4\xB6\xAB\xCE\xF7\xB5\xC4\xC0\xED\xD3\xC9\xA1\xA3"
#define TXT_SHOP_OPT_1           "\xC4\xC7\xC4\xE3\xB0\xEF\xCE\xD2\xCC\xF4\xBD\xF1\xCC\xEC\xB5\xC4\xB7\xE7\xB8\xF1"
#define TXT_SHOP_OPT_2           "\xCE\xD2\xC3\xC7\xCF\xC8\xC8\xA5\xBF\xB4\xBA\xC3\xB3\xD4\xB5\xC4"
#define TXT_SHOP_OPT_3           "\xC4\xE3\xBB\xE1\xB2\xBB\xBB\xE1\xCD\xB5\xCD\xB5\xBF\xE4\xCE\xD2"
#define TXT_GAME_REPLY           "\xBF\xAA\xBE\xD6\xD6\xAE\xC7\xB0\xCF\xC8\xBB\xF7\xB8\xF6\xD5\xC6\xA3\xAC\xCE\xD2\xCF\xEB\xCC\xFD\xC4\xE3\xB0\xB2\xC5\xC5\xD5\xBD\xCA\xF5\xA1\xA3"
#define TXT_GAME_OPT_1           "\xC4\xC7\xC4\xE3\xCF\xEB\xCD\xE6\xCA\xB2\xC3\xB4\xCE\xBB\xD6\xC3"
#define TXT_GAME_OPT_2           "\xCA\xE4\xC1\xCB\xBF\xC9\xB2\xBB\xD0\xED\xC9\xFA\xC6\xF8\xC5\xB6"
#define TXT_GAME_OPT_3           "\xCE\xD2\xC0\xB4\xB1\xA3\xBB\xA4\xC4\xE3"
#define TXT_NIGHT_REPLY          "\xD2\xB9\xC0\xEF\xCA\xCA\xBA\xCF\xC2\xFD\xD2\xBB\xB5\xE3\xCB\xB5\xBB\xB0\xA3\xAC\xCE\xD2\xBB\xE1\xC8\xCF\xD5\xE6\xCC\xFD\xC4\xE3\xB5\xC4\xD0\xA1\xD0\xC4\xCA\xC2\xA1\xA3"
#define TXT_NIGHT_OPT_1          "\xC4\xC7\xCE\xD2\xCF\xC8\xCB\xB5\xBD\xF1\xCC\xEC\xD7\xEE\xC0\xDB\xB5\xC4\xCA\xC2"
#define TXT_NIGHT_OPT_2          "\xC4\xE3\xCF\xC8\xB6\xD4\xCE\xD2\xCB\xB5\xBE\xE4\xCD\xED\xB0\xB2"
#define TXT_NIGHT_OPT_3          "\xC4\xE3\xB1\xA7\xB1\xA7\xCE\xD2\xBA\xC3\xB2\xBB\xBA\xC3"
#define TXT_GEN_REPLY            "\xCE\xD2\xCA\xD5\xB5\xBD\xC0\xB2\xA3\xAC\xD5\xE2\xB8\xF6\xB3\xA1\xBE\xB0\xC0\xEF\xCE\xD2\xBB\xE1\xBC\xCC\xD0\xF8\xCB\xB3\xD7\xC5\xC4\xE3\xB8\xD5\xB2\xC5\xB5\xC4\xBB\xB0\xBD\xD3\xCF\xC2\xC8\xA5\xA1\xA3"
#define TXT_GEN_OPT_1            "\xC4\xC7\xC4\xE3\xBC\xCC\xD0\xF8\xCB\xB5\xCF\xC2\xC8\xA5"
#define TXT_GEN_OPT_2            "\xCE\xD2\xD2\xB2\xB2\xB9\xB3\xE4\xD2\xBB\xB5\xE3\xD7\xD4\xBC\xBA\xB5\xC4\xCF\xEB\xB7\xA8"
#define TXT_GEN_OPT_3            "\xBB\xBB\xD2\xBB\xB8\xF6\xB8\xFC\xD3\xD0\xC8\xA4\xB5\xC4\xBB\xA5\xB6\xAF"

#define TXT_COVER_SUB            "\xBF\xC9\xB0\xAE\xBB\xA5\xB6\xAF\xBD\xC7\xC9\xAB\xCF\xB5\xCD\xB3"
#define TXT_COVER_TITLE          "\x41\x49\xD0\xE9\xC4\xE2\xBB\xEF\xB0\xE9"
#define TXT_COVER_START          "\xBF\xAA\xCA\xBC\xD3\xCE\xCF\xB7"
#define TXT_COVER_HINT           "\x41\x49\xA3\xAC\xBD\xF1\xCC\xEC\xCF\xEB\xBA\xCD\xCE\xD2\xC8\xA5\xC4\xC4\xD2\xBB\xB8\xF6\xB3\xA1\xBE\xB0\xA3\xBF"
#define TXT_SCENE_TITLE          "\xD1\xA1\xD4\xF1\xB3\xA1\xBE\xB0\x20\x2F\x20\xBB\xB0\xCC\xE2"
#define TXT_SCENE_HINT           "\x4B\x31\xC7\xD0\xBB\xBB\x20\x20\x4B\x32\xBD\xF8\xC8\xEB\x20\x20\xB3\xA4\xB0\xB4\xB7\xB5\xBB\xD8"
#define TXT_CHAT_HINT            "\x4B\x31\xC7\xD0\xBB\xBB\x20\x20\x4B\x32\xC8\xB7\xB6\xA8\x20\x20\xB3\xA4\xB0\xB4\x4B\x32\xB7\xB5\xBB\xD8\xB3\xA1\xBE\xB0"


typedef enum

{

    PAGE_COVER = 0,

    PAGE_SCENE_SELECT,

    PAGE_CHAT

} AppPage;



typedef struct

{

    const char *name;

    const char *intro;

    const char *topic_key;

    AvatarState avatar;

} SceneInfo;



typedef struct

{

    char user_text[MAX_TEXT_LEN];
    char ai_text[MAX_TEXT_LEN];

    char options[MAX_OPTION_COUNT][MAX_TEXT_LEN];

    uint8_t option_count;

    uint8_t selected_option;

    uint8_t round;

} ChatState;



typedef struct

{

    const char *text;

    AvatarState avatar;

} IdleFeedback;



static SceneInfo g_scenes[MAX_SCENE_COUNT] =

{

    {TXT_SCENE_SHOPPING, TXT_SCENE_SHOPPING_INTRO, "shopping", AVATAR_HAPPY},
    {TXT_SCENE_GAMING, TXT_SCENE_GAMING_INTRO, "gaming", AVATAR_CURIOUS},
    {TXT_SCENE_CAFE, TXT_SCENE_CAFE_INTRO, "cafe", AVATAR_GENTLE},
    {TXT_SCENE_LIBRARY, TXT_SCENE_LIBRARY_INTRO, "library", AVATAR_GENTLE},
    {TXT_SCENE_NIGHT, TXT_SCENE_NIGHT_INTRO, "night", AVATAR_SHY},
    {TXT_SCENE_ENCOURAGE, TXT_SCENE_ENCOURAGE_INTRO, "encourage", AVATAR_HAPPY},
    {TXT_SCENE_WALK, TXT_SCENE_WALK_INTRO, "walk", AVATAR_CURIOUS},
    {TXT_SCENE_DINNER, TXT_SCENE_DINNER_INTRO, "dinner", AVATAR_HAPPY},
    {TXT_SCENE_MOVIE, TXT_SCENE_MOVIE_INTRO, "movie", AVATAR_CURIOUS},
    {TXT_SCENE_MUSIC, TXT_SCENE_MUSIC_INTRO, "music", AVATAR_GENTLE},
    {TXT_SCENE_EXAM, TXT_SCENE_EXAM_INTRO, "exam", AVATAR_GENTLE},
    {TXT_SCENE_REST, TXT_SCENE_REST_INTRO, "rest", AVATAR_SHY}
};



static IdleFeedback g_idle_feedbacks[] =

{

    {TXT_IDLE_1, AVATAR_GENTLE},
    {TXT_IDLE_2, AVATAR_HAPPY},
    {TXT_IDLE_3, AVATAR_SHY},
    {TXT_IDLE_4, AVATAR_CURIOUS}
};



#define SCENE_COUNT             (sizeof(g_scenes) / sizeof(g_scenes[0]))

#define IDLE_FEEDBACK_NUM       (sizeof(g_idle_feedbacks) / sizeof(g_idle_feedbacks[0]))



static AppPage g_page = PAGE_COVER;

static uint8_t g_scene_index = 0;

static uint8_t g_scene_scroll = 0;

static uint8_t g_idle_feedback_index = 0;

static uint32_t g_idle_count = 0;

static ChatState g_chat_state;



static void Delay(__IO uint32_t nCount);
static void System_Init_All(void);
static void App_MainLoop(void);
static void AI_UI_Init(void);
static void AI_UI_DrawCover(void);
static void AI_UI_DrawSceneSelect(void);
static void AI_UI_DrawChat(void);
static void AI_UI_DrawCurrentPage(void);
static void AI_UI_DrawFrame(void);
static void AI_UI_SetAvatar(AvatarState state);
static void AI_UI_ClearDialogArea(void);
static void AI_UI_ShowDialogTexts(void);
static void AI_UI_ShowSceneWindow(void);
static void AI_UI_ShowCoverHint(const char *text);
static void AI_UI_ShowChatHeader(void);
static void AI_UI_ShowUserText(const char *text);
static void AI_UI_ShowAIText_MultiLine(const char *text);
static void AI_UI_ShowChatOptions(void);
static void AI_UI_ShowFooterHint(const char *text);
static void App_SwitchPage(AppPage page);
static void App_StartSelectedScene(void);
static void App_SelectNextScene(void);
static void App_SelectPrevScene(void);
static void App_SelectNextOption(void);
static void App_SelectPrevOption(void);
static void App_ConfirmCurrentSelection(void);
static void App_HandleCoverTap(uint16_t x, uint16_t y);
static void App_HandleSceneTap(uint16_t x, uint16_t y);
static void App_HandleChatTap(uint16_t x, uint16_t y);
static void AI_HandleKeyEvent(KeyEvent key);
static void AI_HandleGestureEvent(GestureType gesture);
static void Chat_ResetForScene(void);
static void Chat_SelectSceneIntro(void);
static void Chat_SendSelectedOption(void);
static void Chat_BuildMockReply(const char *user_choice);
static uint8_t Chat_ParseReply(char *buf);
static AvatarState AvatarState_FromToken(const char *token);
static void USART1_SendString(const char *str);
static uint8_t USART1_ReadLine(char *buf, uint16_t max_len, uint32_t timeout);
static void USART1_ClearRxBuffer(void);
static const char *CopyLineByPixelWidth(const char *src, char *dst, uint16_t max_pixel_width);
static uint16_t TextPixelWidth(const char *text);
static void TrimLineWithEllipsis(char *text, uint16_t max_pixel_width);
static uint8_t WrapTextLines(const char *src, char lines[][MAX_TEXT_LEN], uint8_t max_lines, uint16_t max_pixel_width);
static uint8_t PointInRect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh);
static void SafeStringCopy(char *dst, const char *src, uint16_t max_len);
static void AppendString(char *dst, const char *src, uint16_t max_len);
static char *FindFieldValue(char *buf, const char *field);
static void AI_Idle_Reset(void);
static void AI_Idle_Update(void);
static void AI_Idle_Feedback(void);

int main(void)

{

    System_Init_All();

    App_MainLoop();



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



    Gesture_Init();

    KeyInput_Init();



    AI_UI_Init();



    Delay(0x7FFFFF);

    USART1_ClearRxBuffer();

}



static void App_MainLoop(void)

{

    while (1)

    {

        KeyEvent key;

        GestureType gesture;

        uint8_t has_input = 0;



        key = KeyInput_Update();

        gesture = Gesture_Update();



        if (key != KEY_EVENT_NONE)

        {

            has_input = 1;

            AI_Idle_Reset();

        }



        if (gesture != GESTURE_NONE)

        {

            has_input = 1;

            AI_Idle_Reset();

        }



        AI_HandleKeyEvent(key);

        AI_HandleGestureEvent(gesture);



        if (!has_input)

        {

            AI_Idle_Update();

        }

    }

}



static void AI_HandleKeyEvent(KeyEvent key)

{

    switch (g_page)

    {

        case PAGE_COVER:

            if (key == KEY_EVENT_K1_SHORT || key == KEY_EVENT_K2_SHORT)

            {

                App_SwitchPage(PAGE_SCENE_SELECT);

            }

            break;



        case PAGE_SCENE_SELECT:

            switch (key)

            {

                case KEY_EVENT_K1_SHORT:

                    App_SelectNextScene();

                    break;

                case KEY_EVENT_K1_LONG:

                    App_SelectPrevScene();

                    break;

                case KEY_EVENT_K2_SHORT:

                    App_StartSelectedScene();

                    break;

                case KEY_EVENT_K2_LONG:

                    App_SwitchPage(PAGE_COVER);

                    break;

                default:

                    break;

            }

            break;



        case PAGE_CHAT:

            switch (key)

            {

                case KEY_EVENT_K1_SHORT:

                    App_SelectNextOption();

                    break;

                case KEY_EVENT_K1_LONG:

                    App_SelectPrevOption();

                    break;

                case KEY_EVENT_K2_SHORT:

                    App_ConfirmCurrentSelection();

                    break;

                case KEY_EVENT_K2_LONG:

                    App_SwitchPage(PAGE_SCENE_SELECT);

                    break;

                default:

                    break;

            }

            break;



        default:

            break;

    }

}



static void AI_HandleGestureEvent(GestureType gesture)

{

    uint8_t has_point;

    uint16_t touch_x;

    uint16_t touch_y;



    has_point = Gesture_GetLastPoint(&touch_x, &touch_y);



    switch (g_page)

    {

        case PAGE_COVER:

            if (gesture == GESTURE_TAP && has_point)

            {

                App_HandleCoverTap(touch_x, touch_y);

            }

            else if (gesture == GESTURE_SWIPE_UP || gesture == GESTURE_SWIPE_LEFT)

            {

                App_SwitchPage(PAGE_SCENE_SELECT);

            }

            break;



        case PAGE_SCENE_SELECT:

            if (gesture == GESTURE_TAP && has_point)

            {

                App_HandleSceneTap(touch_x, touch_y);

            }

            else if (gesture == GESTURE_SWIPE_UP)

            {

                App_SelectNextScene();

            }

            else if (gesture == GESTURE_SWIPE_DOWN)

            {

                App_SelectPrevScene();

            }

            else if (gesture == GESTURE_SWIPE_RIGHT)

            {

                App_StartSelectedScene();

            }

            else if (gesture == GESTURE_LONG_PRESS)

            {

                App_SwitchPage(PAGE_COVER);

            }

            break;



        case PAGE_CHAT:

            if (gesture == GESTURE_TAP && has_point)

            {

                App_HandleChatTap(touch_x, touch_y);

            }

            else if (gesture == GESTURE_SWIPE_UP)

            {

                App_SelectNextOption();

            }

            else if (gesture == GESTURE_SWIPE_DOWN)

            {

                App_SelectPrevOption();

            }

            else if (gesture == GESTURE_SWIPE_RIGHT)

            {

                App_ConfirmCurrentSelection();

            }

            else if (gesture == GESTURE_LONG_PRESS)

            {

                App_SwitchPage(PAGE_SCENE_SELECT);

            }

            break;



        default:

            break;

    }

}



static void AI_UI_Init(void)

{

    Chat_ResetForScene();

    AI_UI_DrawCurrentPage();

}



static void App_SwitchPage(AppPage page)

{

    g_page = page;



    if (g_page == PAGE_SCENE_SELECT)

    {

        if (g_scene_index >= SCENE_COUNT)

        {

            g_scene_index = 0;

        }



        if (g_scene_index < g_scene_scroll)

        {

            g_scene_scroll = g_scene_index;

        }

    }



    AI_UI_DrawCurrentPage();

}



static void App_StartSelectedScene(void)

{

    Chat_ResetForScene();

    g_page = PAGE_CHAT;
    AI_UI_DrawCurrentPage();

    Chat_SelectSceneIntro();

}



static void App_SelectNextScene(void)

{

    if (g_scene_index + 1 >= SCENE_COUNT)

    {

        g_scene_index = 0;

        g_scene_scroll = 0;

    }

    else

    {

        g_scene_index++;

        if (g_scene_index >= g_scene_scroll + SCENE_VISIBLE_COUNT)

        {

            g_scene_scroll++;

        }

    }



    AI_UI_ShowSceneWindow();

}



static void App_SelectPrevScene(void)

{

    if (g_scene_index == 0)

    {

        g_scene_index = (uint8_t)(SCENE_COUNT - 1);

        if (SCENE_COUNT > SCENE_VISIBLE_COUNT)

        {

            g_scene_scroll = (uint8_t)(SCENE_COUNT - SCENE_VISIBLE_COUNT);

        }

    }

    else

    {

        g_scene_index--;

        if (g_scene_index < g_scene_scroll)

        {

            g_scene_scroll--;

        }

    }



    AI_UI_ShowSceneWindow();

}



static void App_SelectNextOption(void)

{

    if (g_chat_state.option_count == 0)

    {

        return;

    }



    g_chat_state.selected_option++;

    if (g_chat_state.selected_option >= g_chat_state.option_count)

    {

        g_chat_state.selected_option = 0;

    }



    AI_UI_ShowChatOptions();

}



static void App_SelectPrevOption(void)

{

    if (g_chat_state.option_count == 0)

    {

        return;

    }



    if (g_chat_state.selected_option == 0)

    {

        g_chat_state.selected_option = (uint8_t)(g_chat_state.option_count - 1);

    }

    else

    {

        g_chat_state.selected_option--;

    }



    AI_UI_ShowChatOptions();

}



static void App_ConfirmCurrentSelection(void)

{

    if (g_page == PAGE_CHAT)

    {

        Chat_SendSelectedOption();

    }

}



static void App_HandleCoverTap(uint16_t x, uint16_t y)

{

    if (PointInRect(x, y, COVER_BTN_X, COVER_BTN_Y, COVER_BTN_W, COVER_BTN_H))

    {

        App_SwitchPage(PAGE_SCENE_SELECT);

    }

}



static void App_HandleSceneTap(uint16_t x, uint16_t y)

{

    uint8_t row;

    uint8_t scene_id;



    if (!PointInRect(x, y, SCENE_LIST_X, SCENE_LIST_Y,

        SCENE_ITEM_W, SCENE_ITEM_H * SCENE_VISIBLE_COUNT))

    {

        return;

    }



    row = (uint8_t)((y - SCENE_LIST_Y) / SCENE_ITEM_H);

    scene_id = (uint8_t)(g_scene_scroll + row);



    if (scene_id < SCENE_COUNT)

    {

        g_scene_index = scene_id;

        AI_UI_ShowSceneWindow();

        App_StartSelectedScene();

    }

}



static void App_HandleChatTap(uint16_t x, uint16_t y)

{

    uint8_t row;



    if (!PointInRect(x, y, CHAT_OPTION_X, CHAT_OPTION_Y,

        CHAT_OPTION_W, CHAT_OPTION_H * MAX_OPTION_COUNT))

    {

        return;

    }



    row = (uint8_t)((y - CHAT_OPTION_Y) / CHAT_OPTION_H);



    if (row < g_chat_state.option_count)

    {

        g_chat_state.selected_option = row;

        AI_UI_ShowChatOptions();

        Chat_SendSelectedOption();

    }

}



static void Chat_ResetForScene(void)

{

    g_chat_state.round = 0;

    g_chat_state.selected_option = 0;

    g_chat_state.option_count = 0;
    g_chat_state.user_text[0] = '\0';
    g_chat_state.ai_text[0] = '\0';
    g_chat_state.options[0][0] = '\0';
    g_chat_state.options[1][0] = '\0';
    g_chat_state.options[2][0] = '\0';
}



static void Chat_SelectSceneIntro(void)

{

    char send_buf[MAX_PROTOCOL_BUF];
    char user_line[MAX_TEXT_LEN];



    AI_UI_SetAvatar(g_scenes[g_scene_index].avatar);
    SafeStringCopy(user_line, TXT_YOU_PREFIX, MAX_TEXT_LEN);
    AppendString(user_line, g_scenes[g_scene_index].name, MAX_TEXT_LEN);
    g_chat_state.ai_text[0] = '\0';
    AI_UI_ShowUserText(user_line);

    AI_UI_ShowAIText_MultiLine(TXT_LOADING);


    SafeStringCopy(send_buf, "SCENE:", MAX_PROTOCOL_BUF);

    AppendString(send_buf, g_scenes[g_scene_index].topic_key, MAX_PROTOCOL_BUF);

    AppendString(send_buf, "\n", MAX_PROTOCOL_BUF);



    USART1_ClearRxBuffer();

    USART1_SendString(send_buf);



    if (!USART1_ReadLine(send_buf, sizeof(send_buf), SCENE_REPLY_TIMEOUT) || !Chat_ParseReply(send_buf))

    {

        SafeStringCopy(g_chat_state.ai_text, TXT_CHAT_SCENE_OK, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[0], TXT_OPT_SCENE_1, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[1], TXT_OPT_SCENE_2, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[2], TXT_OPT_SCENE_3, MAX_TEXT_LEN);
        g_chat_state.option_count = 3;

        g_chat_state.selected_option = 0;

    }



    AI_UI_DrawChat();

}



static void Chat_SendSelectedOption(void)

{

    char send_buf[MAX_PROTOCOL_BUF];

    char user_line[MAX_TEXT_LEN];

    const char *user_choice;



    if (g_chat_state.option_count == 0)

    {

        return;

    }



    user_choice = g_chat_state.options[g_chat_state.selected_option];



    SafeStringCopy(user_line, TXT_YOU_PREFIX, MAX_TEXT_LEN);
    AppendString(user_line, user_choice, MAX_TEXT_LEN);



    g_chat_state.ai_text[0] = '\0';
    AI_UI_ShowUserText(user_line);

    AI_UI_SetAvatar(AVATAR_THINKING);

    AI_UI_ShowAIText_MultiLine(TXT_THINKING);


    SafeStringCopy(send_buf, "SCENE:", MAX_PROTOCOL_BUF);

    AppendString(send_buf, g_scenes[g_scene_index].topic_key, MAX_PROTOCOL_BUF);

    AppendString(send_buf, "|IDX:", MAX_PROTOCOL_BUF);
    {
        uint16_t len;
        len = (uint16_t)strlen(send_buf);
        if (len < (uint16_t)(MAX_PROTOCOL_BUF - 1))
        {
            send_buf[len] = (char)('0' + g_chat_state.selected_option);
            send_buf[len + 1] = '\0';
        }
    }

    AppendString(send_buf, "\n", MAX_PROTOCOL_BUF);



    USART1_ClearRxBuffer();

    USART1_SendString(send_buf);



    if (!USART1_ReadLine(send_buf, sizeof(send_buf), CHAT_REPLY_TIMEOUT) || !Chat_ParseReply(send_buf))

    {

        Chat_BuildMockReply(user_choice);

    }



    AI_UI_SetAvatar(g_scenes[g_scene_index].avatar);

    AI_UI_ShowAIText_MultiLine(g_chat_state.ai_text);

    AI_UI_ShowChatOptions();

}



static void Chat_BuildMockReply(const char *user_choice)

{

    SafeStringCopy(g_chat_state.ai_text, "\x41\x49\xA3\xBA", MAX_TEXT_LEN);



    if (strcmp(g_scenes[g_scene_index].topic_key, "shopping") == 0)

    {

        AppendString(g_chat_state.ai_text, TXT_SHOP_REPLY, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[0], TXT_SHOP_OPT_1, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[1], TXT_SHOP_OPT_2, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[2], TXT_SHOP_OPT_3, MAX_TEXT_LEN);
    }

    else if (strcmp(g_scenes[g_scene_index].topic_key, "gaming") == 0)

    {

        AppendString(g_chat_state.ai_text, TXT_GAME_REPLY, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[0], TXT_GAME_OPT_1, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[1], TXT_GAME_OPT_2, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[2], TXT_GAME_OPT_3, MAX_TEXT_LEN);
    }

    else if (strcmp(g_scenes[g_scene_index].topic_key, "night") == 0)

    {

        AppendString(g_chat_state.ai_text, TXT_NIGHT_REPLY, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[0], TXT_NIGHT_OPT_1, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[1], TXT_NIGHT_OPT_2, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[2], TXT_NIGHT_OPT_3, MAX_TEXT_LEN);
    }

    else

    {

        AppendString(g_chat_state.ai_text, TXT_GEN_REPLY, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[0], TXT_GEN_OPT_1, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[1], TXT_GEN_OPT_2, MAX_TEXT_LEN);
        SafeStringCopy(g_chat_state.options[2], TXT_GEN_OPT_3, MAX_TEXT_LEN);
    }



    if (user_choice[0] != '\0')

    {

        g_chat_state.round++;

    }



    g_chat_state.option_count = 3;

    g_chat_state.selected_option = 0;

}



static uint8_t Chat_ParseReply(char *buf)

{
    char *text;
    char *opt1;
    char *opt2;
    char *opt3;
    char *avatar;
    char *end;

    text = strstr(buf, "TEXT=");
    opt1 = strstr(buf, "OPT1=");
    opt2 = strstr(buf, "OPT2=");
    opt3 = strstr(buf, "OPT3=");
    avatar = strstr(buf, "AVATAR=");

    if (text == 0 || opt1 == 0)
    {
        return 0;
    }

    text += 5;
    opt1 += 5;
    if (opt2 != 0)
    {
        opt2 += 5;
    }
    if (opt3 != 0)
    {
        opt3 += 5;
    }
    if (avatar != 0)
    {
        avatar += 7;
    }

    end = strchr(text, '|');
    if (end != 0)
    {
        *end = '\0';
    }
    end = strchr(opt1, '|');
    if (end != 0)
    {
        *end = '\0';
    }
    if (opt2 != 0)
    {
        end = strchr(opt2, '|');
        if (end != 0)
        {
            *end = '\0';
        }
    }
    if (opt3 != 0)
    {
        end = strchr(opt3, '|');
        if (end != 0)
        {
            *end = '\0';
        }
    }
    if (avatar != 0)
    {
        end = strchr(avatar, '|');
        if (end != 0)
        {
            *end = '\0';
        }
    }



    SafeStringCopy(g_chat_state.ai_text, "\x41\x49\xA3\xBA", MAX_TEXT_LEN);

    AppendString(g_chat_state.ai_text, text, MAX_TEXT_LEN);



    SafeStringCopy(g_chat_state.options[0], opt1, MAX_TEXT_LEN);

    g_chat_state.option_count = 1;



    if (opt2 != 0 && opt2[0] != '\0')

    {

        SafeStringCopy(g_chat_state.options[1], opt2, MAX_TEXT_LEN);

        g_chat_state.option_count = 2;

    }



    if (opt3 != 0 && opt3[0] != '\0')

    {

        SafeStringCopy(g_chat_state.options[2], opt3, MAX_TEXT_LEN);

        g_chat_state.option_count = 3;

    }



    if (avatar != 0)

    {

        g_scenes[g_scene_index].avatar = AvatarState_FromToken(avatar);

    }



    g_chat_state.selected_option = 0;

    return 1;

}



static AvatarState AvatarState_FromToken(const char *token)

{

    if (strcmp(token, "happy") == 0)

    {

        return AVATAR_HAPPY;

    }

    if (strcmp(token, "shy") == 0)

    {

        return AVATAR_SHY;

    }

    if (strcmp(token, "gentle") == 0)

    {

        return AVATAR_GENTLE;

    }

    if (strcmp(token, "thinking") == 0)

    {

        return AVATAR_THINKING;

    }

    if (strcmp(token, "tired") == 0)

    {

        return AVATAR_TIRED;

    }



    return AVATAR_CURIOUS;

}



static void AI_UI_DrawCurrentPage(void)

{

    switch (g_page)

    {

        case PAGE_COVER:

            AI_UI_DrawCover();

            break;

        case PAGE_SCENE_SELECT:

            AI_UI_DrawSceneSelect();

            break;

        case PAGE_CHAT:

            AI_UI_DrawChat();

            break;

        default:

            AI_UI_DrawCover();

            break;

    }

}



static void AI_UI_DrawCover(void)

{

    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);



    AI_UI_DrawFrame();

    AI_UI_SetAvatar(AVATAR_HAPPY);



    LCD_SetColors(COLOR_TITLE, COLOR_BG);

    ILI9341_DispString_EN_CH(54, 176, TXT_COVER_TITLE);



    LCD_SetColors(COLOR_AI_TEXT, COLOR_BG);

    ILI9341_DispString_EN_CH(28, 212, TXT_COVER_SUB);


    LCD_SetTextColor(COLOR_HIGHLIGHT);

    ILI9341_DrawRectangle(COVER_BTN_X, COVER_BTN_Y, COVER_BTN_W, COVER_BTN_H, 0);



    LCD_SetColors(COLOR_HIGHLIGHT, COLOR_BG);

    ILI9341_DispString_EN_CH(78, 259, TXT_COVER_START);


    AI_UI_ShowCoverHint(TXT_COVER_HINT);
}



static void AI_UI_DrawSceneSelect(void)

{

    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);



    LCD_SetColors(COLOR_TITLE, COLOR_BG);

    ILI9341_DispString_EN_CH(16, 12, TXT_SCENE_TITLE);


    LCD_SetColors(COLOR_HINT, COLOR_BG);

    ILI9341_DispString_EN_CH(10, 34, TXT_SCENE_HINT);


    AI_UI_ShowSceneWindow();

}



static void AI_UI_DrawChat(void)

{

    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);



    AI_UI_DrawFrame();

    AI_UI_SetAvatar(g_scenes[g_scene_index].avatar);

    AI_UI_ShowChatHeader();

    AI_UI_ShowDialogTexts();

    AI_UI_ShowChatOptions();

}



static void AI_UI_DrawFrame(void)

{

    LCD_SetTextColor(COLOR_FRAME);

    ILI9341_DrawRectangle(AVATAR_X - 2, AVATAR_Y - 2,

        AVATAR_DISP_W + 4, AVATAR_DISP_H + 4, 0);



    ILI9341_DrawRectangle(CHAT_DIALOG_X, CHAT_DIALOG_Y,

        CHAT_DIALOG_W, CHAT_DIALOG_H, 0);



    ILI9341_DrawLine(0, 244, LCD_X_LENGTH, 244);
}



static void AI_UI_ShowSceneWindow(void)

{

    uint8_t i;

    uint8_t scene_id;

    uint16_t y;
    char intro_line[MAX_TEXT_LEN];



    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 56, LCD_X_LENGTH, 236);



    for (i = 0; i < SCENE_VISIBLE_COUNT; i++)

    {

        scene_id = (uint8_t)(g_scene_scroll + i);

        y = (uint16_t)(SCENE_LIST_Y + i * SCENE_ITEM_H);



        if (scene_id >= SCENE_COUNT)

        {

            break;

        }



        if (scene_id == g_scene_index)

        {

            LCD_SetTextColor(COLOR_HIGHLIGHT);

            ILI9341_DrawRectangle(SCENE_LIST_X, y, SCENE_ITEM_W, SCENE_ITEM_H - 2, 0);

            LCD_SetColors(COLOR_HIGHLIGHT, COLOR_BG);

        }

        else

        {

            LCD_SetTextColor(COLOR_FRAME);

            ILI9341_DrawRectangle(SCENE_LIST_X, y, SCENE_ITEM_W, SCENE_ITEM_H - 2, 0);

            LCD_SetColors(COLOR_PANEL, COLOR_BG);

        }



    ILI9341_DispString_EN_CH(14, y + 6, (char *)g_scenes[scene_id].name);

    }



    SafeStringCopy(intro_line, g_scenes[g_scene_index].intro, MAX_TEXT_LEN);
    TrimLineWithEllipsis(intro_line, 216);

    LCD_SetColors(COLOR_AI_TEXT, COLOR_BG);

    ILI9341_DispString_EN_CH(12, 270, intro_line);

}



static void AI_UI_ShowCoverHint(const char *text)

{
    char hint_line[MAX_TEXT_LEN];

    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 294, LCD_X_LENGTH, 22);

    SafeStringCopy(hint_line, text, MAX_TEXT_LEN);
    TrimLineWithEllipsis(hint_line, 220);



    LCD_SetColors(COLOR_HINT, COLOR_BG);

    ILI9341_DispString_EN_CH(10, 298, hint_line);

}



static void AI_UI_ShowChatHeader(void)

{

    LCD_SetColors(COLOR_TITLE, COLOR_BG);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, 22);
    ILI9341_DispString_EN_CH(12, 6, (char *)g_scenes[g_scene_index].name);
}



static void AI_UI_SetAvatar(AvatarState state)

{

    const uint16_t *img;
    uint16_t dst_x;
    uint16_t dst_y;
    uint16_t src_x;
    uint16_t src_y;



    img = AI_Avatar_GetImage(state);



    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(AVATAR_X, AVATAR_Y, AVATAR_DISP_W, AVATAR_DISP_H);

    ILI9341_OpenWindow(AVATAR_X, AVATAR_Y, AVATAR_DISP_W, AVATAR_DISP_H);



    LCD_CMD = CMD_SetPixel;



    for (dst_y = 0; dst_y < AVATAR_DISP_H; dst_y++)
    {
        src_y = (uint16_t)((uint32_t)dst_y * AI_AVATAR_HEIGHT / AVATAR_DISP_H);

        for (dst_x = 0; dst_x < AVATAR_DISP_W; dst_x++)
        {
            src_x = (uint16_t)((uint32_t)dst_x * AI_AVATAR_WIDTH / AVATAR_DISP_W);
            LCD_DATA = img[src_y * AI_AVATAR_WIDTH + src_x];
        }
    }

}



static void AI_UI_ClearDialogArea(void)

{

    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(CHAT_DIALOG_X + 2, CHAT_DIALOG_Y + 2, CHAT_DIALOG_W - 4, CHAT_DIALOG_H - 4);

}



static void AI_UI_ShowDialogTexts(void)

{

    char lines[2][MAX_TEXT_LEN];

    uint8_t line_count;

    uint8_t i;



    AI_UI_ClearDialogArea();

    if (g_chat_state.user_text[0] != '\0')
    {
        line_count = WrapTextLines(g_chat_state.user_text, lines, 2, UI_TEXT_WIDTH);

        LCD_SetColors(COLOR_USER_TEXT, COLOR_BG);

        for (i = 0; i < line_count; i++)

        {

            ILI9341_DispString_EN_CH(CHAT_DIALOG_X + CHAT_TEXT_X_OFFSET,

                CHAT_DIALOG_Y + CHAT_USER_Y_OFFSET + i * CHAT_LINE_STEP,

                lines[i]);

        }
    }

    if (g_chat_state.ai_text[0] != '\0')
    {
        line_count = WrapTextLines(g_chat_state.ai_text, lines, 2, UI_TEXT_WIDTH);

        LCD_SetColors(COLOR_AI_TEXT, COLOR_BG);

        for (i = 0; i < line_count; i++)

        {

            ILI9341_DispString_EN_CH(CHAT_DIALOG_X + CHAT_TEXT_X_OFFSET,

                CHAT_DIALOG_Y + CHAT_AI_Y_OFFSET + i * CHAT_LINE_STEP,

                lines[i]);

        }
    }
}




static void AI_UI_ShowUserText(const char *text)

{

    SafeStringCopy(g_chat_state.user_text, text, MAX_TEXT_LEN);
    AI_UI_ShowDialogTexts();

}




static void AI_UI_ShowAIText_MultiLine(const char *text)

{

    SafeStringCopy(g_chat_state.ai_text, text, MAX_TEXT_LEN);
    AI_UI_ShowDialogTexts();

}





static void AI_UI_ShowChatOptions(void)

{

    uint8_t i;

    uint16_t y;

    char option_line[MAX_TEXT_LEN];



    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 242, LCD_X_LENGTH, 78);



    for (i = 0; i < g_chat_state.option_count; i++)

    {

        y = (uint16_t)(CHAT_OPTION_Y + i * CHAT_OPTION_H);



        if (i == g_chat_state.selected_option)

        {

            LCD_SetTextColor(COLOR_HIGHLIGHT);

            ILI9341_DrawRectangle(CHAT_OPTION_X, y, CHAT_OPTION_W, CHAT_OPTION_H - 2, 0);

            LCD_SetColors(COLOR_HIGHLIGHT, COLOR_BG);

        }

        else

        {

            LCD_SetTextColor(COLOR_FRAME);

            ILI9341_DrawRectangle(CHAT_OPTION_X, y, CHAT_OPTION_W, CHAT_OPTION_H - 2, 0);

            LCD_SetColors(COLOR_PANEL, COLOR_BG);

        }



        SafeStringCopy(option_line, g_chat_state.options[i], MAX_TEXT_LEN);

        TrimLineWithEllipsis(option_line, UI_TEXT_WIDTH);

        ILI9341_DispString_EN_CH(CHAT_OPTION_X + CHAT_TEXT_X_OFFSET, y + 3, option_line);

    }



    AI_UI_ShowFooterHint("");

}





static void AI_UI_ShowFooterHint(const char *text)

{
    if (text[0] == '\0')

    {

        return;

    }

    LCD_SetColors(COLOR_PANEL, COLOR_BG);

    ILI9341_Clear(0, 298, LCD_X_LENGTH, 20);


    LCD_SetColors(COLOR_HINT, COLOR_BG);

    ILI9341_DispString_EN_CH(6, 302, (char *)text);

}




static void AI_Idle_Reset(void)

{

    g_idle_count = 0;

}



static void AI_Idle_Feedback(void)

{

    IdleFeedback *item;



    if (g_page != PAGE_COVER)

    {

        return;

    }



    item = &g_idle_feedbacks[g_idle_feedback_index];



    AI_UI_SetAvatar(item->avatar);

    AI_UI_ShowCoverHint(item->text);



    g_idle_feedback_index++;

    if (g_idle_feedback_index >= IDLE_FEEDBACK_NUM)

    {

        g_idle_feedback_index = 0;

    }

}



static void AI_Idle_Update(void)

{

    g_idle_count++;



    if (g_idle_count >= IDLE_FEEDBACK_COUNT)

    {

        g_idle_count = 0;

        AI_Idle_Feedback();

    }

}



static const char *CopyLineByPixelWidth(const char *src, char *dst, uint16_t max_pixel_width)

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



static uint16_t TextPixelWidth(const char *text)

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



static void TrimLineWithEllipsis(char *text, uint16_t max_pixel_width)

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



static uint8_t WrapTextLines(const char *src, char lines[][MAX_TEXT_LEN], uint8_t max_lines, uint16_t max_pixel_width)

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



static uint8_t PointInRect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)

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



static void SafeStringCopy(char *dst, const char *src, uint16_t max_len)

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



static void AppendString(char *dst, const char *src, uint16_t max_len)

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



static char *FindFieldValue(char *buf, const char *field)

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



static void USART1_SendString(const char *str)

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



static uint8_t USART1_ReadLine(char *buf, uint16_t max_len, uint32_t timeout)

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



static void USART1_ClearRxBuffer(void)

{

    while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_RXNE) != RESET)

    {

        USART_ReceiveData(DEBUG_USARTx);

    }

}



static void Delay(__IO uint32_t nCount)

{

    while (nCount--)

    {

    }

}

