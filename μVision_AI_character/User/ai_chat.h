#ifndef __AI_CHAT_H
#define __AI_CHAT_H

#include "ai_app_data.h"

typedef struct
{
    SceneInfo *scenes;
    uint8_t scene_index;
    ChatState *chat_state;
    void (*ui_set_avatar)(AvatarState state);
    void (*ui_show_user_text)(const char *text);
    void (*ui_show_ai_text)(const char *text);
    void (*ui_show_chat_options)(void);
    void (*ui_draw_current_page)(void);
} AIChatContext;

void AI_Chat_ResetForScene(AIChatContext *ctx);
void AI_Chat_SelectSceneIntro(AIChatContext *ctx);
void AI_Chat_SendSelectedOption(AIChatContext *ctx);

#endif
