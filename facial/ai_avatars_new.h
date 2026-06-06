#ifndef __AI_AVATARS_NEW_H
#define __AI_AVATARS_NEW_H

#include <stdint.h>

typedef enum {
    AVATAR_HAPPY = 0,
    AVATAR_SHY,
    AVATAR_GENTLE,
    AVATAR_THINKING,
    AVATAR_CURIOUS
} AvatarState;

#define AI_AVATAR_WIDTH  160
#define AI_AVATAR_HEIGHT 120

/* Composite positions (relative to body top-left) */
#define LEFT_EYE_X  55
#define LEFT_EYE_Y  31
#define RIGHT_EYE_X 89
#define RIGHT_EYE_Y 31
#define MOUTH_X     62
#define MOUTH_Y     46

void AI_Avatar_Init(void);
void AI_Avatar_RenderToBuffer(AvatarState state, uint16_t *buffer);

#endif /* __AI_AVATARS_NEW_H */
