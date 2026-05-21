#ifndef __AI_AVATARS_H
#define __AI_AVATARS_H

#include "stm32f10x.h"

/*
 * AI Avatar Pack - 200x150 RGB565
 * Generated for STM32 ILI9341 display.
 * Each image size: 200 x 150 x 2 = 60000 bytes.
 * Total image data: 420000 bytes.
 */
#define AI_AVATAR_WIDTH   200
#define AI_AVATAR_HEIGHT  150
#define AI_AVATAR_PIXELS  (AI_AVATAR_WIDTH * AI_AVATAR_HEIGHT)

typedef enum
{
    AVATAR_NORMAL_IDLE = 0,
    AVATAR_THINKING,
    AVATAR_HAPPY,
    AVATAR_TIRED,
    AVATAR_CURIOUS,
    AVATAR_GENTLE,
    AVATAR_SHY,
    AVATAR_COUNT
} AvatarState;

extern const uint16_t gAvatar_NormalIdle[AI_AVATAR_PIXELS];
extern const uint16_t gAvatar_Thinking[AI_AVATAR_PIXELS];
extern const uint16_t gAvatar_Happy[AI_AVATAR_PIXELS];
extern const uint16_t gAvatar_Tired[AI_AVATAR_PIXELS];
extern const uint16_t gAvatar_Curious[AI_AVATAR_PIXELS];
extern const uint16_t gAvatar_Gentle[AI_AVATAR_PIXELS];
extern const uint16_t gAvatar_Shy[AI_AVATAR_PIXELS];

const uint16_t *AI_Avatar_GetImage(AvatarState state);

#endif /* __AI_AVATARS_H */
