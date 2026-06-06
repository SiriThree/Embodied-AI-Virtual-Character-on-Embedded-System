#ifndef __AI_AVATAR_DATA_H
#define __AI_AVATAR_DATA_H

#include <stdint.h>

extern const uint16_t avatar_body_body[19200];
#define AVATAR_BODY_BODY_WIDTH 160
#define AVATAR_BODY_BODY_HEIGHT 120

extern const uint16_t avatar_eye_eye_angry[187];
extern const uint16_t avatar_eye_eye_halfclosed[187];
extern const uint16_t avatar_eye_eye_halfmoon[187];
extern const uint16_t avatar_eye_eye_normal[187];
extern const uint16_t avatar_eye_eye_open[187];
extern const uint16_t avatar_eye_eye_sad[187];
extern const uint16_t avatar_eye_eye_sagging[187];
extern const uint16_t avatar_eye_eye_squint[187];
#define AVATAR_EYE_WIDTH 17
#define AVATAR_EYE_HEIGHT 11

extern const uint16_t avatar_mouth_mouth_bigO[1120];
extern const uint16_t avatar_mouth_mouth_flat[1120];
extern const uint16_t avatar_mouth_mouth_laugh[1120];
extern const uint16_t avatar_mouth_mouth_reserve_arch[1120];
extern const uint16_t avatar_mouth_mouth_saw[1120];
extern const uint16_t avatar_mouth_mouth_smallO[1120];
extern const uint16_t avatar_mouth_mouth_smile[1120];
extern const uint16_t avatar_mouth_mouth_wave[1120];
#define AVATAR_MOUTH_WIDTH 40
#define AVATAR_MOUTH_HEIGHT 28

#endif /* __AI_AVATAR_DATA_H */