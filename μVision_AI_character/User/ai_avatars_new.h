#ifndef __AI_AVATARS_NEW_H
#define __AI_AVATARS_NEW_H

#include <stdint.h>
#include "emotion.h"

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
void AI_Avatar_RenderToBuffer(FaceID_t face, uint16_t *buffer);

#endif /* __AI_AVATARS_NEW_H */
