#ifndef __AI_UI_H
#define __AI_UI_H

#include "ai_app_data.h"

typedef struct
{
    AppPage page;
    uint8_t scene_index;
    uint8_t scene_scroll;
    const SceneInfo *scenes;
    uint8_t scene_count;
    ChatState *chat_state;
} AIUIContext;

void AI_UI_Init(const AIUIContext *ctx);
void AI_UI_DrawCurrentPage(const AIUIContext *ctx);
void AI_UI_ShowSceneWindow(const AIUIContext *ctx);
void AI_UI_ShowCoverHint(const char *text);
void AI_UI_ShowChatHeader(const AIUIContext *ctx);
void AI_UI_SetAvatar(AvatarState state);
void AI_UI_ShowUserText(ChatState *chat_state, const char *text);
void AI_UI_ShowAIText_MultiLine(ChatState *chat_state, const char *text);
void AI_UI_ShowChatOptions(const ChatState *chat_state);
void AI_UI_ShowFooterHint(const char *text);

#endif
