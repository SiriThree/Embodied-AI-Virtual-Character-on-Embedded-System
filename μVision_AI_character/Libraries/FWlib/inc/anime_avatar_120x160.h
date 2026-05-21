#ifndef __ANIME_AVATAR_120X160_H
#define __ANIME_AVATAR_120X160_H

#include "stm32f10x.h"

#define ANIME_AVATAR_WIDTH   120
#define ANIME_AVATAR_HEIGHT  160

/*
 * Image format:
 * - RGB565
 * - Row-major order
 * - Size: 120 x 160
 * - Data length: 19200 pixels, 38400 bytes
 */
extern const uint16_t gImage_anime_avatar_120x160[ANIME_AVATAR_WIDTH * ANIME_AVATAR_HEIGHT];

static void AI_UI_DrawAvatar(uint16_t x, uint16_t y);

#endif
