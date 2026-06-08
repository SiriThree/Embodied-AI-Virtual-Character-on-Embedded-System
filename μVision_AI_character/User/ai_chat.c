/* Chat layer: serial protocol, reply parsing and local mock fallback. */
#include <string.h>

#include "ai_app_utils.h"
#include "ai_chat.h"

static void Chat_BuildMockReply(AIChatContext *ctx, const char *user_choice);
static uint8_t Chat_ParseReply(AIChatContext *ctx, char *buf);

#define TXT_MOCK_REPLY_SHOP      "\xB9\xFE\xBB\xF9\xC3\xD7\xA1\xAB\xC2\xFD\xC2\xFD\xB9\xE4\xD1\xBD\xA1\xA3"
#define TXT_MOCK_REPLY_GAME      "\xC2\xFC\xB2\xA8\xC2\xFC\xB2\xA8\xA1\xAB\xB1\xF0\xBC\xB1\xD1\xBD\xA1\xA3"
#define TXT_MOCK_REPLY_NIGHT     "\xCC\xF0\xCC\xF0\xB5\xC4\xA3\xAC\xC2\xFD\xC2\xFD\xCB\xB5\xD1\xBD\xA1\xA3"
#define TXT_MOCK_REPLY_GEN       "\xB9\xFE\xBB\xF9\xC3\xD7\xA1\xAB\xBC\xCC\xD0\xF8\xC5\xE3\xC4\xE3\xA1\xA3"

#define TXT_MOCK_OPT_NEAR        "\xD4\xD9\xBF\xBF\xBD\xFC\xD2\xBB\xB5\xE3"
#define TXT_MOCK_OPT_MAMBO       "\xC2\xFC\xB2\xA8\xD2\xA1\xD2\xBB\xCF\xC2"
#define TXT_MOCK_OPT_SWEET       "\xBB\xBB\xB8\xF6\xCC\xF0\xCC\xF0\xBB\xB0\xCC\xE2"
#define TXT_MOCK_OPT_LISTEN      "\xC8\xC3\xCE\xD2\xC2\xFD\xC2\xFD\xCC\xFD"
#define TXT_MOCK_OPT_CHAT        "\xD2\xBB\xC6\xF0\xC7\xE1\xC7\xE1\xC1\xC4"
#define TXT_MOCK_OPT_MORE        "\xD4\xD9\xB8\xF8\xCE\xD2\xD2\xBB\xBE\xE4"

void AI_Chat_ResetForScene(AIChatContext *ctx)
{
    ctx->chat_state->round = 0;
    ctx->chat_state->selected_option = 0;
    ctx->chat_state->option_count = 0;
    ctx->chat_state->user_text[0] = '\0';
    ctx->chat_state->ai_text[0] = '\0';
    ctx->chat_state->options[0][0] = '\0';
    ctx->chat_state->options[1][0] = '\0';
    ctx->chat_state->options[2][0] = '\0';
}

void AI_Chat_SelectSceneIntro(AIChatContext *ctx)
{
    char send_buf[MAX_PROTOCOL_BUF];
    char user_line[MAX_TEXT_LEN];

    ctx->ui_set_avatar(Emotion_GetFace());
    SafeStringCopy(user_line, TXT_YOU_PREFIX, MAX_TEXT_LEN);
    AppendString(user_line, ctx->scenes[ctx->scene_index].name, MAX_TEXT_LEN);
    ctx->chat_state->ai_text[0] = '\0';
    ctx->ui_show_user_text(user_line);
    ctx->ui_show_ai_text(TXT_LOADING);

    SafeStringCopy(send_buf, "SCENE:", MAX_PROTOCOL_BUF);
    AppendString(send_buf, ctx->scenes[ctx->scene_index].topic_key, MAX_PROTOCOL_BUF);
    AppendString(send_buf, "\n", MAX_PROTOCOL_BUF);

    USART1_ClearRxBuffer();
    USART1_SendString(send_buf);

    if (!USART1_ReadLine(send_buf, sizeof(send_buf), SCENE_REPLY_TIMEOUT) || !Chat_ParseReply(ctx, send_buf))
    {
        SafeStringCopy(ctx->chat_state->ai_text, TXT_CHAT_SCENE_OK, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[0], TXT_OPT_SCENE_1, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[1], TXT_OPT_SCENE_2, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[2], TXT_OPT_SCENE_3, MAX_TEXT_LEN);
        ctx->chat_state->option_count = 3;
        ctx->chat_state->selected_option = 0;
    }

    ctx->ui_draw_current_page();
}

void AI_Chat_SendSelectedOption(AIChatContext *ctx)
{
    char send_buf[MAX_PROTOCOL_BUF];
    char user_line[MAX_TEXT_LEN];
    const char *user_choice;
    uint16_t len;

    if (ctx->chat_state->option_count == 0)
    {
        return;
    }

    user_choice = ctx->chat_state->options[ctx->chat_state->selected_option];

    SafeStringCopy(user_line, TXT_YOU_PREFIX, MAX_TEXT_LEN);
    AppendString(user_line, user_choice, MAX_TEXT_LEN);

    ctx->chat_state->ai_text[0] = '\0';
    ctx->ui_show_user_text(user_line);
    ctx->ui_set_avatar(FACE_THINKING);
    ctx->ui_show_ai_text(TXT_THINKING);

    SafeStringCopy(send_buf, "SCENE:", MAX_PROTOCOL_BUF);
    AppendString(send_buf, ctx->scenes[ctx->scene_index].topic_key, MAX_PROTOCOL_BUF);
    AppendString(send_buf, "|IDX:", MAX_PROTOCOL_BUF);

    len = (uint16_t)strlen(send_buf);
    if (len < (uint16_t)(MAX_PROTOCOL_BUF - 1))
    {
        send_buf[len] = (char)('0' + ctx->chat_state->selected_option);
        send_buf[len + 1] = '\0';
    }

    AppendString(send_buf, "\n", MAX_PROTOCOL_BUF);

    USART1_ClearRxBuffer();
    USART1_SendString(send_buf);

    if (!USART1_ReadLine(send_buf, sizeof(send_buf), CHAT_REPLY_TIMEOUT) || !Chat_ParseReply(ctx, send_buf))
    {
        Chat_BuildMockReply(ctx, user_choice);
    }

    ctx->ui_set_avatar(Emotion_GetFace());
    ctx->ui_show_ai_text(ctx->chat_state->ai_text);
    ctx->ui_show_chat_options();
}

static void Chat_BuildMockReply(AIChatContext *ctx, const char *user_choice)
{
    SafeStringCopy(ctx->chat_state->ai_text, "\x41\x49\xA3\xBA", MAX_TEXT_LEN);

    if (strcmp(ctx->scenes[ctx->scene_index].topic_key, "shopping") == 0)
    {
        ctx->scenes[ctx->scene_index].avatar = AVATAR_HAPPY;
        AppendString(ctx->chat_state->ai_text, TXT_MOCK_REPLY_SHOP, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[0], TXT_MOCK_OPT_NEAR, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[1], TXT_MOCK_OPT_MAMBO, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[2], TXT_MOCK_OPT_SWEET, MAX_TEXT_LEN);
    }
    else if (strcmp(ctx->scenes[ctx->scene_index].topic_key, "gaming") == 0)
    {
        ctx->scenes[ctx->scene_index].avatar = AVATAR_CURIOUS;
        AppendString(ctx->chat_state->ai_text, TXT_MOCK_REPLY_GAME, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[0], TXT_MOCK_OPT_LISTEN, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[1], TXT_MOCK_OPT_MAMBO, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[2], TXT_MOCK_OPT_MORE, MAX_TEXT_LEN);
    }
    else if (strcmp(ctx->scenes[ctx->scene_index].topic_key, "night") == 0)
    {
        ctx->scenes[ctx->scene_index].avatar = AVATAR_GENTLE;
        AppendString(ctx->chat_state->ai_text, TXT_MOCK_REPLY_NIGHT, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[0], TXT_MOCK_OPT_CHAT, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[1], TXT_MOCK_OPT_LISTEN, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[2], TXT_MOCK_OPT_MORE, MAX_TEXT_LEN);
    }
    else
    {
        ctx->scenes[ctx->scene_index].avatar = AVATAR_CURIOUS;
        AppendString(ctx->chat_state->ai_text, TXT_MOCK_REPLY_GEN, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[0], TXT_MOCK_OPT_NEAR, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[1], TXT_MOCK_OPT_CHAT, MAX_TEXT_LEN);
        SafeStringCopy(ctx->chat_state->options[2], TXT_MOCK_OPT_SWEET, MAX_TEXT_LEN);
    }

    if (user_choice[0] != '\0')
    {
        ctx->chat_state->round++;
    }

    ctx->chat_state->option_count = 3;
    ctx->chat_state->selected_option = 0;
}

static uint8_t Chat_ParseReply(AIChatContext *ctx, char *buf)
{
    char *text;
    char *opt1;
    char *opt2;
    char *opt3;
    char *avatar;
    char *avatar_token;
    char *opt3_token;
    char *opt2_token;
    char *opt1_token;

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

    avatar_token = strstr(buf, "|AVATAR=");
    opt3_token = strstr(buf, "|OPT3=");
    opt2_token = strstr(buf, "|OPT2=");
    opt1_token = strstr(buf, "|OPT1=");

    if (opt1_token != 0)
    {
        *opt1_token = '\0';
    }
    if (opt2_token != 0)
    {
        *opt2_token = '\0';
    }
    if (opt3_token != 0)
    {
        *opt3_token = '\0';
    }
    if (avatar_token != 0)
    {
        *avatar_token = '\0';
    }

    SafeStringCopy(ctx->chat_state->ai_text, "\x41\x49\xA3\xBA", MAX_TEXT_LEN);
    AppendString(ctx->chat_state->ai_text, text, MAX_TEXT_LEN);

    SafeStringCopy(ctx->chat_state->options[0], opt1, MAX_TEXT_LEN);
    ctx->chat_state->option_count = 1;

    if (opt2 != 0 && opt2[0] != '\0')
    {
        SafeStringCopy(ctx->chat_state->options[1], opt2, MAX_TEXT_LEN);
        ctx->chat_state->option_count = 2;
    }

    if (opt3 != 0 && opt3[0] != '\0')
    {
        SafeStringCopy(ctx->chat_state->options[2], opt3, MAX_TEXT_LEN);
        ctx->chat_state->option_count = 3;
    }

    ctx->chat_state->selected_option = 0;
    return 1;
}

void AI_Chat_SendVolume(uint8_t vol)
{
    char vol_buf[16];
    char num_str[4];
	int i = 0;
    
    if (vol > 100) vol = 100;

    SafeStringCopy(vol_buf, "SET_VOL:", 16);

    
    if (vol == 100) {
        num_str[i++] = '1';
        num_str[i++] = '0';
        num_str[i++] = '0';
    } else {
        if (vol >= 10) num_str[i++] = (vol / 10) + '0';
        num_str[i++] = (vol % 10) + '0';
    }
    num_str[i] = '\0';

    AppendString(vol_buf, num_str, 16);
    AppendString(vol_buf, "\n", 16);
    USART1_SendString(vol_buf);
}

void AI_Chat_SendAudioStop(void)
{
    USART1_SendString("AUDIO_STOP\n");
}