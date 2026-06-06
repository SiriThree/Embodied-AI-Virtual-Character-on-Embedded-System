/* App layer: page state machine, key/gesture dispatch and idle behavior. */
#include "stm32f10x.h"

#include "gesture.h"
#include "key_input.h"
#include "ai_app_data.h"
#include "ai_app_utils.h"
#include "ai_ui.h"
#include "ai_chat.h"
#include "ai_app.h"
#include "emotion.h"

static AppPage g_page = PAGE_COVER;
static AppPage g_prev_page = PAGE_COVER;
static uint8_t g_scene_index = 0;
static uint8_t g_scene_scroll = 0;
static uint8_t g_idle_feedback_index = 0;
static uint32_t g_idle_count = 0;
static uint32_t g_emotion_update_count = 0;
static ChatState g_chat_state;

static AIUIContext AI_UI_BuildContext(void);
static AIChatContext AI_Chat_BuildContext(void);
static void UI_Init(void);
static void UI_DrawCurrentPage(void);
static void UI_ShowSceneWindow(void);
static void UI_ShowCoverHintText(const char *text);
static void UI_SetAvatarState(FaceID_t face);
static void UI_ShowUserText(const char *text);
static void UI_ShowAIText(const char *text);
static void UI_ShowChatOptions(void);
static void Chat_ResetForScene(void);
static void Chat_SelectSceneIntro(void);
static void Chat_SendSelectedOption(void);
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
static void AI_Idle_Reset(void);
static void AI_Idle_Update(void);
static void AI_Idle_Feedback(void);

void AI_App_Init(void)
{
    UI_Init();
}

void AI_App_Run(void)
{
    while (1)
    {
        KeyEvent key;
        GestureType gesture;
        uint8_t has_input;

        has_input = 0;
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

            /* Trigger emotion events based on gesture */
            if (gesture == GESTURE_TAP) {
                Emotion_Event(EVENT_TOUCH_TAP, 1.0f);
            } else if (gesture == GESTURE_LONG_PRESS) {
                Emotion_Event(EVENT_TOUCH_LONG, 1.0f);
            } else if (gesture == GESTURE_SWIPE_LEFT || gesture == GESTURE_SWIPE_RIGHT) {
                Emotion_Event(EVENT_TOUCH_SWIPE_H, 1.0f);
            } else if (gesture == GESTURE_SWIPE_UP || gesture == GESTURE_SWIPE_DOWN) {
                Emotion_Event(EVENT_TOUCH_SWIPE_V, 1.0f);
            }
        }

        AI_HandleKeyEvent(key);
        AI_HandleGestureEvent(gesture);

        if (!has_input)
        {
            AI_Idle_Update();
        }

        /* Periodic emotion update (~100ms) */
        g_emotion_update_count++;
        if (g_emotion_update_count >= 10000)
        {
            g_emotion_update_count = 0;
            Emotion_Update();
        }
    }
}

static AIUIContext AI_UI_BuildContext(void)
{
    AIUIContext ctx;

    ctx.page = g_page;
    ctx.scene_index = g_scene_index;
    ctx.scene_scroll = g_scene_scroll;
    ctx.scenes = g_scenes;
    ctx.scene_count = g_scene_count;
    ctx.chat_state = &g_chat_state;

    return ctx;
}

static AIChatContext AI_Chat_BuildContext(void)
{
    AIChatContext ctx;

    ctx.scenes = g_scenes;
    ctx.scene_index = g_scene_index;
    ctx.chat_state = &g_chat_state;
    ctx.ui_set_avatar = UI_SetAvatarState;
    ctx.ui_show_user_text = UI_ShowUserText;
    ctx.ui_show_ai_text = UI_ShowAIText;
    ctx.ui_show_chat_options = UI_ShowChatOptions;
    ctx.ui_draw_current_page = UI_DrawCurrentPage;

    return ctx;
}

static void UI_Init(void)
{
    AIUIContext ctx;
    Chat_ResetForScene();
    ctx = AI_UI_BuildContext();
    AI_UI_Init(&ctx);
}

static void UI_DrawCurrentPage(void)
{
    AIUIContext ctx;
    ctx = AI_UI_BuildContext();
    AI_UI_DrawCurrentPage(&ctx);
}

static void UI_ShowSceneWindow(void)
{
    AIUIContext ctx;
    ctx = AI_UI_BuildContext();
    AI_UI_ShowSceneWindow(&ctx);
}

static void UI_ShowCoverHintText(const char *text)
{
    AI_UI_ShowCoverHint(text);
}

static void UI_SetAvatarState(FaceID_t face)
{
    AI_UI_SetAvatar(face);
}

static void UI_ShowUserText(const char *text)
{
    AI_UI_ShowUserText(&g_chat_state, text);
}

static void UI_ShowAIText(const char *text)
{
    AI_UI_ShowAIText_MultiLine(&g_chat_state, text);
}

static void UI_ShowChatOptions(void)
{
    AI_UI_ShowChatOptions(&g_chat_state);
}

static void Chat_ResetForScene(void)
{
    AIChatContext ctx;
    ctx = AI_Chat_BuildContext();
    AI_Chat_ResetForScene(&ctx);
}

static void Chat_SelectSceneIntro(void)
{
    AIChatContext ctx;
    ctx = AI_Chat_BuildContext();
    AI_Chat_SelectSceneIntro(&ctx);
}

static void Chat_SendSelectedOption(void)
{
    AIChatContext ctx;
    ctx = AI_Chat_BuildContext();
    AI_Chat_SendSelectedOption(&ctx);
}

static void AI_HandleKeyEvent(KeyEvent key)
{
    /* K1 long press: toggle emotion page from any page */
    if (key == KEY_EVENT_K1_LONG)
    {
        if (g_page == PAGE_EMOTION)
        {
            App_SwitchPage(g_prev_page);
        }
        else
        {
            g_prev_page = g_page;
            App_SwitchPage(PAGE_EMOTION);
        }
        return;
    }

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

        case PAGE_EMOTION:
            if (key == KEY_EVENT_K2_LONG)
            {
                App_SwitchPage(g_prev_page);
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
            else if (gesture == GESTURE_SWIPE_DOWN)
            {
                App_SwitchPage(PAGE_EMOTION);
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

        case PAGE_EMOTION:
            if (gesture == GESTURE_SWIPE_UP || gesture == GESTURE_LONG_PRESS)
            {
                App_SwitchPage(PAGE_COVER);
            }
            break;

        default:
            break;
    }
}

static void App_SwitchPage(AppPage page)
{
    g_page = page;

    if (g_page == PAGE_SCENE_SELECT)
    {
        if (g_scene_index >= g_scene_count)
        {
            g_scene_index = 0;
        }

        if (g_scene_index < g_scene_scroll)
        {
            g_scene_scroll = g_scene_index;
        }
    }

    UI_DrawCurrentPage();
}

static void App_StartSelectedScene(void)
{
    Chat_ResetForScene();
    g_page = PAGE_CHAT;
    UI_DrawCurrentPage();
    Chat_SelectSceneIntro();
}

static void App_SelectNextScene(void)
{
    if (g_scene_index + 1 >= g_scene_count)
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

    UI_ShowSceneWindow();
}

static void App_SelectPrevScene(void)
{
    if (g_scene_index == 0)
    {
        g_scene_index = (uint8_t)(g_scene_count - 1);
        if (g_scene_count > SCENE_VISIBLE_COUNT)
        {
            g_scene_scroll = (uint8_t)(g_scene_count - SCENE_VISIBLE_COUNT);
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

    UI_ShowSceneWindow();
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

    UI_ShowChatOptions();
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

    UI_ShowChatOptions();
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

    if (!PointInRect(x, y, SCENE_LIST_X, SCENE_LIST_Y, SCENE_ITEM_W, SCENE_ITEM_H * SCENE_VISIBLE_COUNT))
    {
        return;
    }

    row = (uint8_t)((y - SCENE_LIST_Y) / SCENE_ITEM_H);
    scene_id = (uint8_t)(g_scene_scroll + row);

    if (scene_id < g_scene_count)
    {
        g_scene_index = scene_id;
        UI_ShowSceneWindow();
        App_StartSelectedScene();
    }
}

static void App_HandleChatTap(uint16_t x, uint16_t y)
{
    uint8_t row;

    if (!PointInRect(x, y, CHAT_OPTION_X, CHAT_OPTION_Y, CHAT_OPTION_W, CHAT_OPTION_H * MAX_OPTION_COUNT))
    {
        return;
    }

    row = (uint8_t)((y - CHAT_OPTION_Y) / CHAT_OPTION_H);

    if (row < g_chat_state.option_count)
    {
        g_chat_state.selected_option = row;
        UI_ShowChatOptions();
        Chat_SendSelectedOption();
    }
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
    UI_DrawCurrentPage();
    UI_ShowCoverHintText(item->text);

    g_idle_feedback_index++;
    if (g_idle_feedback_index >= g_idle_feedback_count)
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
