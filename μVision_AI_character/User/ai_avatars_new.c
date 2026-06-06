#include "ai_avatars_new.h"
#include "ai_avatar_data.h"
#include <string.h>

typedef struct {
    const uint16_t *eye;
    const uint16_t *mouth;
} AvatarComposition;

static const AvatarComposition g_avatar_map[] = {
    {avatar_eye_eye_open, avatar_mouth_mouth_smile},           /* FACE_HAPPY */
    {avatar_eye_eye_halfclosed, avatar_mouth_mouth_smile},     /* FACE_CONTENT */
    {avatar_eye_eye_halfmoon, avatar_mouth_mouth_wave},        /* FACE_RELAXED */
    {avatar_eye_eye_open, avatar_mouth_mouth_bigO},            /* FACE_SURPRISED */
    {avatar_eye_eye_normal, avatar_mouth_mouth_flat},          /* FACE_NEUTRAL */
    {avatar_eye_eye_halfclosed, avatar_mouth_mouth_flat},      /* FACE_BORED */
    {avatar_eye_eye_angry, avatar_mouth_mouth_saw},            /* FACE_ANGRY */
    {avatar_eye_eye_sad, avatar_mouth_mouth_reserve_arch},     /* FACE_SAD */
    {avatar_eye_eye_sagging, avatar_mouth_mouth_flat}          /* FACE_DEPRESSED */
};

void AI_Avatar_Init(void)
{
    /* No initialization needed */
}

static void BlitImage(uint16_t *dst_buffer, const uint16_t *src,
                      uint16_t dst_x, uint16_t dst_y,
                      uint16_t src_w, uint16_t src_h,
                      uint16_t dst_stride)
{
    uint16_t y, x;
    for (y = 0; y < src_h; y++) {
        uint16_t *dst_row = dst_buffer + (dst_y + y) * dst_stride + dst_x;
        const uint16_t *src_row = src + y * src_w;
        for (x = 0; x < src_w; x++) {
            dst_row[x] = src_row[x];
        }
    }
}

void AI_Avatar_RenderToBuffer(FaceID_t face, uint16_t *buffer)
{
    const AvatarComposition *comp;

    if (face >= sizeof(g_avatar_map) / sizeof(g_avatar_map[0])) {
        face = FACE_NEUTRAL;
    }

    comp = &g_avatar_map[face];

    /* Copy body base */
    memcpy(buffer, avatar_body_body, sizeof(uint16_t) * AI_AVATAR_WIDTH * AI_AVATAR_HEIGHT);

    /* Blit left eye */
    BlitImage(buffer, comp->eye, LEFT_EYE_X, LEFT_EYE_Y,
              AVATAR_EYE_WIDTH, AVATAR_EYE_HEIGHT, AI_AVATAR_WIDTH);

    /* Blit right eye */
    BlitImage(buffer, comp->eye, RIGHT_EYE_X, RIGHT_EYE_Y,
              AVATAR_EYE_WIDTH, AVATAR_EYE_HEIGHT, AI_AVATAR_WIDTH);

    /* Blit mouth */
    BlitImage(buffer, comp->mouth, MOUTH_X, MOUTH_Y,
              AVATAR_MOUTH_WIDTH, AVATAR_MOUTH_HEIGHT, AI_AVATAR_WIDTH);
}
