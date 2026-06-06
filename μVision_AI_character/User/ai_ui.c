/* UI layer: all screen drawing, avatar rendering and text/option presentation. */
#include "stm32f10x.h"

#include "./lcd/bsp_ili9341_lcd.h"

#include "ai_avatars.h"
#include "ai_app_utils.h"
#include "ai_ui.h"

#define LCD_CMD   (*((volatile uint16_t *)FSMC_Addr_ILI9341_CMD))
#define LCD_DATA  (*((volatile uint16_t *)FSMC_Addr_ILI9341_DATA))

#define CHAT_AVATAR_X  30
#define CHAT_AVATAR_Y  42
#define CHAT_AVATAR_W  180
#define CHAT_AVATAR_H  126

static void AI_UI_DrawCover(void);
static void AI_UI_DrawSceneSelect(const AIUIContext *ctx);
static void AI_UI_DrawChat(const AIUIContext *ctx);
static void AI_UI_DrawFrame(void);
static void AI_UI_ClearDialogArea(void);
static void AI_UI_ShowDialogTexts(const ChatState *chat_state);
static void AI_UI_DrawCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    uint16_t fill_color, uint16_t border_color);
static void AI_UI_DrawSelectionCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    uint8_t selected);
static void AI_UI_DrawSoftBackground(void);
static void AI_UI_DrawAvatarScaled(AvatarState state, uint16_t x, uint16_t y,
    uint16_t w, uint16_t h, uint16_t bg_color);
static const char *AI_UI_GetAvatarBadge(AvatarState state);
static const char *AI_UI_GetSceneBadge(const SceneInfo *scene);
static void AI_UI_BuildSceneCounter(char *buf, uint8_t current, uint8_t total);
static uint16_t AI_UI_CenterX(const char *text);

void AI_UI_Init(const AIUIContext *ctx)
{
    AI_UI_DrawCurrentPage(ctx);
}

void AI_UI_DrawCurrentPage(const AIUIContext *ctx)
{
    switch (ctx->page)
    {
        case PAGE_COVER:
            AI_UI_DrawCover();
            break;

        case PAGE_SCENE_SELECT:
            AI_UI_DrawSceneSelect(ctx);
            break;

        case PAGE_CHAT:
            AI_UI_DrawChat(ctx);
            break;

        default:
            AI_UI_DrawCover();
            break;
    }
}

static void AI_UI_DrawCover(void)
{
    const uint16_t cover_avatar_x = 20;
    const uint16_t cover_avatar_y = 18;
    const uint16_t cover_avatar_w = 200;
    const uint16_t cover_avatar_h = 150;

    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);

    AI_UI_DrawSoftBackground();
    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(cover_avatar_x - 1, cover_avatar_y - 1, cover_avatar_w + 2, cover_avatar_h + 2);
    AI_UI_DrawCard(18, 14, 70, 18, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);
    LCD_SetColors(COLOR_HINT, COLOR_PANEL_DARK);
    ILI9341_DispString_EN_CH(26, 18, "PROLOGUE");
    LCD_SetTextColor(COLOR_FRAME_SOFT);
    ILI9341_DrawRectangle(cover_avatar_x, cover_avatar_y, cover_avatar_w, cover_avatar_h, 0);

    AI_UI_DrawAvatarScaled(AVATAR_HAPPY, cover_avatar_x + 4, cover_avatar_y + 4,
        cover_avatar_w - 8, cover_avatar_h - 8, COLOR_PANEL_DARK);

    LCD_SetColors(COLOR_TITLE, COLOR_BG);
    ILI9341_DispString_EN_CH(AI_UI_CenterX(TXT_COVER_TITLE), 176, TXT_COVER_TITLE);
    LCD_SetTextColor(COLOR_TITLE);
    ILI9341_DrawLine(74, 198, 166, 198);

    LCD_SetColors(COLOR_SUBTITLE, COLOR_BG);
    ILI9341_DispString_EN_CH(AI_UI_CenterX(TXT_COVER_SUB), 208, TXT_COVER_SUB);

    AI_UI_DrawCard(COVER_BTN_X, COVER_BTN_Y, COVER_BTN_W, COVER_BTN_H,
        COLOR_HIGHLIGHT_FILL, COLOR_HIGHLIGHT);
    LCD_SetTextColor(COLOR_HIGHLIGHT);
    ILI9341_DrawRectangle(COVER_BTN_X + 5, COVER_BTN_Y + 5, COVER_BTN_W - 10, COVER_BTN_H - 10, 0);

    LCD_SetColors(COLOR_HIGHLIGHT, COLOR_HIGHLIGHT_FILL);
    ILI9341_DispString_EN_CH(AI_UI_CenterX(TXT_COVER_START), 250, TXT_COVER_START);

    LCD_SetColors(COLOR_HINT, COLOR_BG);
    ILI9341_DispString_EN_CH(AI_UI_CenterX("K2 / TOUCH"), 286, "K2 / TOUCH");
}

static void AI_UI_DrawSceneSelect(const AIUIContext *ctx)
{
    char counter[8];

    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);

    AI_UI_DrawSoftBackground();
    AI_UI_BuildSceneCounter(counter, (uint8_t)(ctx->scene_index + 1), ctx->scene_count);

    LCD_SetColors(COLOR_TITLE, COLOR_BG);
    ILI9341_DispString_EN_CH(18, 14, TXT_SCENE_TITLE);

    AI_UI_DrawCard(176, 12, 42, 18, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);
    LCD_SetColors(COLOR_HINT, COLOR_PANEL_DARK);
    ILI9341_DispString_EN_CH(184, 16, counter);

    LCD_SetColors(COLOR_HINT, COLOR_BG);
    ILI9341_DispString_EN_CH(18, 35, TXT_SCENE_HINT);
    LCD_SetTextColor(COLOR_FRAME_SOFT);
    ILI9341_DrawLine(14, 50, 226, 50);

    AI_UI_ShowSceneWindow(ctx);
}

static void AI_UI_DrawChat(const AIUIContext *ctx)
{
    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);

    AI_UI_DrawSoftBackground();
    AI_UI_ShowChatHeader(ctx);
    AI_UI_DrawFrame();
    AI_UI_SetAvatar(ctx->scenes[ctx->scene_index].avatar);
    AI_UI_ShowDialogTexts(ctx->chat_state);
    AI_UI_ShowChatOptions(ctx->chat_state);
}

static void AI_UI_DrawFrame(void)
{
    AI_UI_DrawCard(CHAT_AVATAR_X - 4, CHAT_AVATAR_Y - 4, CHAT_AVATAR_W + 8, CHAT_AVATAR_H + 8,
        COLOR_PANEL_DARK, COLOR_FRAME);
    LCD_SetTextColor(COLOR_FRAME);
    ILI9341_DrawRectangle(CHAT_AVATAR_X - 1, CHAT_AVATAR_Y - 1, CHAT_AVATAR_W + 2, CHAT_AVATAR_H + 2, 0);

    AI_UI_DrawCard(CHAT_DIALOG_X, CHAT_DIALOG_Y, CHAT_DIALOG_W, CHAT_DIALOG_H,
        COLOR_PANEL_DARK, COLOR_FRAME);
}

void AI_UI_ShowSceneWindow(const AIUIContext *ctx)
{
    uint8_t row;
    uint8_t scene_id;
    uint16_t y;
    char intro_line[MAX_TEXT_LEN];
    const char *badge;

    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(0, 54, LCD_X_LENGTH, 258);

    for (row = 0; row < SCENE_VISIBLE_COUNT; row++)
    {
        scene_id = (uint8_t)(ctx->scene_scroll + row);
        if (scene_id >= ctx->scene_count)
        {
            break;
        }

        y = (uint16_t)(SCENE_LIST_Y + row * SCENE_ITEM_H);

        AI_UI_DrawSelectionCard(SCENE_LIST_X, y, SCENE_ITEM_W, SCENE_ITEM_H - 4,
            (uint8_t)(scene_id == ctx->scene_index));

        if (scene_id == ctx->scene_index)
        {
            LCD_SetColors(COLOR_HIGHLIGHT, COLOR_HIGHLIGHT_FILL);
        }
        else
        {
            LCD_SetColors(COLOR_AI_TEXT, COLOR_PANEL_DARK);
        }

        ILI9341_DispString_EN_CH(22, y + 8, (char *)ctx->scenes[scene_id].name);
    }

    SafeStringCopy(intro_line, ctx->scenes[ctx->scene_index].intro, MAX_TEXT_LEN);
    TrimLineWithEllipsis(intro_line, 216);
    badge = AI_UI_GetSceneBadge(&ctx->scenes[ctx->scene_index]);

    AI_UI_DrawCard(12, 270, 74, 18, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);
    LCD_SetColors(COLOR_HINT, COLOR_PANEL_DARK);
    ILI9341_DispString_EN_CH(22, 274, (char *)badge);

    AI_UI_DrawCard(12, 290, 216, 20, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);
    LCD_SetColors(COLOR_HINT, COLOR_PANEL_DARK);
    ILI9341_DispString_EN_CH(18, 293, intro_line);
}

void AI_UI_ShowCoverHint(const char *text)
{
    char hint_line[MAX_TEXT_LEN];
    uint16_t x;

    AI_UI_DrawCard(10, 290, 220, 20, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);

    SafeStringCopy(hint_line, text, MAX_TEXT_LEN);
    TrimLineWithEllipsis(hint_line, 196);
    x = AI_UI_CenterX(hint_line);
    if (x < 16)
    {
        x = 16;
    }

    LCD_SetColors(COLOR_HINT, COLOR_PANEL_DARK);
    ILI9341_DispString_EN_CH(x, 294, hint_line);
}

void AI_UI_ShowChatHeader(const AIUIContext *ctx)
{
    const char *badge;

    badge = AI_UI_GetAvatarBadge(ctx->scenes[ctx->scene_index].avatar);

    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, 36);
    LCD_SetColors(COLOR_TITLE, COLOR_BG);
    ILI9341_DispString_EN_CH(18, 14, (char *)ctx->scenes[ctx->scene_index].name);
    LCD_SetTextColor(COLOR_FRAME_SOFT);
    ILI9341_DrawLine(14, 32, 124, 32);

    AI_UI_DrawCard(154, 12, 64, 18, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);
    LCD_SetColors(COLOR_HINT, COLOR_PANEL_DARK);
    ILI9341_DispString_EN_CH(166, 16, (char *)badge);
}

void AI_UI_SetAvatar(AvatarState state)
{
    AI_UI_DrawAvatarScaled(state, CHAT_AVATAR_X, CHAT_AVATAR_Y, CHAT_AVATAR_W, CHAT_AVATAR_H,
        COLOR_PANEL_DARK);
}

static void AI_UI_ClearDialogArea(void)
{
    LCD_SetColors(COLOR_PANEL, COLOR_PANEL_DARK);
    ILI9341_Clear(CHAT_DIALOG_X + 2, CHAT_DIALOG_Y + 2, CHAT_DIALOG_W - 4, CHAT_DIALOG_H - 4);
}

static void AI_UI_ShowDialogTexts(const ChatState *chat_state)
{
    char lines[2][MAX_TEXT_LEN];
    uint8_t line_count;
    uint8_t i;

    AI_UI_ClearDialogArea();

    if (chat_state->user_text[0] != '\0')
    {
        line_count = WrapTextLines(chat_state->user_text, lines, 2, UI_TEXT_WIDTH);

        LCD_SetColors(COLOR_USER_TEXT, COLOR_PANEL_DARK);
        for (i = 0; i < line_count; i++)
        {
            ILI9341_DispString_EN_CH(
                CHAT_DIALOG_X + CHAT_TEXT_X_OFFSET,
                CHAT_DIALOG_Y + CHAT_USER_Y_OFFSET + i * CHAT_LINE_STEP,
                lines[i]);
        }
    }

    if (chat_state->ai_text[0] != '\0')
    {
        line_count = WrapTextLines(chat_state->ai_text, lines, 2, UI_TEXT_WIDTH);

        LCD_SetColors(COLOR_AI_TEXT, COLOR_PANEL_DARK);
        for (i = 0; i < line_count; i++)
        {
            ILI9341_DispString_EN_CH(
                CHAT_DIALOG_X + CHAT_TEXT_X_OFFSET,
                CHAT_DIALOG_Y + CHAT_AI_Y_OFFSET + i * CHAT_LINE_STEP,
                lines[i]);
        }
    }
}

void AI_UI_ShowUserText(ChatState *chat_state, const char *text)
{
    SafeStringCopy(chat_state->user_text, text, MAX_TEXT_LEN);
    AI_UI_ShowDialogTexts(chat_state);
}

void AI_UI_ShowAIText_MultiLine(ChatState *chat_state, const char *text)
{
    SafeStringCopy(chat_state->ai_text, text, MAX_TEXT_LEN);
    AI_UI_ShowDialogTexts(chat_state);
}

void AI_UI_ShowChatOptions(const ChatState *chat_state)
{
    uint8_t i;
    uint16_t y;
    char option_line[MAX_TEXT_LEN];

    LCD_SetColors(COLOR_PANEL, COLOR_BG);
    ILI9341_Clear(0, 244, LCD_X_LENGTH, 76);

    for (i = 0; i < chat_state->option_count; i++)
    {
        y = (uint16_t)(CHAT_OPTION_Y + i * CHAT_OPTION_H);

        AI_UI_DrawSelectionCard(CHAT_OPTION_X, y, CHAT_OPTION_W, CHAT_OPTION_H - 3,
            (uint8_t)(i == chat_state->selected_option));

        SafeStringCopy(option_line, chat_state->options[i], MAX_TEXT_LEN);
        TrimLineWithEllipsis(option_line, UI_TEXT_WIDTH);
        if (i == chat_state->selected_option)
        {
            LCD_SetColors(COLOR_HIGHLIGHT, COLOR_HIGHLIGHT_FILL);
        }
        else
        {
            LCD_SetColors(COLOR_AI_TEXT, COLOR_PANEL_DARK);
        }
        ILI9341_DispString_EN_CH(CHAT_OPTION_X + CHAT_TEXT_X_OFFSET, y + 3, option_line);
    }

    AI_UI_ShowFooterHint("");
}

void AI_UI_ShowFooterHint(const char *text)
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

static void AI_UI_DrawCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    uint16_t fill_color, uint16_t border_color)
{
    LCD_SetTextColor(fill_color);
    ILI9341_DrawRectangle(x, y, w, h, 1);
    LCD_SetTextColor(border_color);
    ILI9341_DrawRectangle(x, y, w, h, 0);
}

static void AI_UI_DrawSelectionCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    uint8_t selected)
{
    if (selected)
    {
        AI_UI_DrawCard(x, y, w, h, COLOR_HIGHLIGHT_FILL, COLOR_HIGHLIGHT);
        LCD_SetTextColor(COLOR_HIGHLIGHT);
        ILI9341_DrawRectangle(x + 3, y + 3, w - 6, h - 6, 0);
    }
    else
    {
        AI_UI_DrawCard(x, y, w, h, COLOR_PANEL_DARK, COLOR_FRAME_SOFT);
    }
}

static void AI_UI_DrawSoftBackground(void)
{
    LCD_SetTextColor(COLOR_PANEL_SOFT);
    ILI9341_DrawRectangle(8, 8, 224, 304, 0);
    LCD_SetTextColor(COLOR_FRAME_SOFT);
    ILI9341_DrawLine(16, 8, 64, 8);
    ILI9341_DrawLine(176, 8, 224, 8);
    ILI9341_DrawLine(8, 300, 56, 300);
    ILI9341_DrawLine(184, 300, 232, 300);
}

static void AI_UI_DrawAvatarScaled(AvatarState state, uint16_t x, uint16_t y,
    uint16_t w, uint16_t h, uint16_t bg_color)
{
    const uint16_t *img;
    uint16_t dst_x;
    uint16_t dst_y;
    uint16_t src_x;
    uint16_t src_y;

    img = AI_Avatar_GetImage(state);

    LCD_SetColors(COLOR_PANEL, bg_color);
    ILI9341_Clear(x, y, w, h);
    ILI9341_OpenWindow(x, y, w, h);

    LCD_CMD = CMD_SetPixel;

    for (dst_y = 0; dst_y < h; dst_y++)
    {
        src_y = (uint16_t)((uint32_t)dst_y * AI_AVATAR_HEIGHT / h);

        for (dst_x = 0; dst_x < w; dst_x++)
        {
            src_x = (uint16_t)((uint32_t)dst_x * AI_AVATAR_WIDTH / w);
            LCD_DATA = img[src_y * AI_AVATAR_WIDTH + src_x];
        }
    }
}

static const char *AI_UI_GetAvatarBadge(AvatarState state)
{
    switch (state)
    {
        case AVATAR_HAPPY:
            return "HAPPY";

        case AVATAR_SHY:
            return "SHY";

        case AVATAR_GENTLE:
            return "GENTLE";

        case AVATAR_CURIOUS:
            return "CURIOUS";

        case AVATAR_THINKING:
            return "THINK";

        default:
            return "MOOD";
    }
}

static const char *AI_UI_GetSceneBadge(const SceneInfo *scene)
{
    if (scene->topic_key[0] == 's')
    {
        return "DATE";
    }
    if (scene->topic_key[0] == 'g')
    {
        return "PLAY";
    }
    if (scene->topic_key[0] == 'c')
    {
        return "CAFE";
    }
    if (scene->topic_key[0] == 'l')
    {
        return "STUDY";
    }
    if (scene->topic_key[0] == 'n')
    {
        return "NIGHT";
    }
    if (scene->topic_key[0] == 'e')
    {
        return "CARE";
    }
    if (scene->topic_key[0] == 'w')
    {
        return "WALK";
    }
    if (scene->topic_key[0] == 'd')
    {
        return "MEAL";
    }
    if (scene->topic_key[0] == 'm')
    {
        return "MUSIC";
    }
    if (scene->topic_key[0] == 'r')
    {
        return "REST";
    }

    return "SCENE";
}

static void AI_UI_BuildSceneCounter(char *buf, uint8_t current, uint8_t total)
{
    buf[0] = (char)('0' + current / 10);
    buf[1] = (char)('0' + current % 10);
    buf[2] = '/';
    buf[3] = (char)('0' + total / 10);
    buf[4] = (char)('0' + total % 10);
    buf[5] = '\0';
}

static uint16_t AI_UI_CenterX(const char *text)
{
    uint16_t width;

    width = TextPixelWidth(text);
    if (width >= LCD_X_LENGTH)
    {
        return 0;
    }

    return (uint16_t)((LCD_X_LENGTH - width) / 2);
}
