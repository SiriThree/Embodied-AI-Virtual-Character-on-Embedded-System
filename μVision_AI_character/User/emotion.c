#include "emotion.h"
#include <math.h>

/* ═══════ 内部状态 ═══════ */
static float    emo_pleasure    = 0.0f;
static float    emo_arousal     = 0.0f;
static FaceID_t emo_face        = FACE_NEUTRAL;

/* 衰减半衰期 (单位: 100ms 步数) */
#define P_HALF_LIFE_STEPS   600u   /* 60s / 0.1s = 600 步 */
#define A_HALF_LIFE_STEPS   400u   /* 40s / 0.1s = 400 步 */

/* 表情判定阈值 */
#define THRESH_HIGH_A  0.3f
#define THRESH_LOW_A  -0.3f
#define THRESH_HIGH_P  0.15f
#define THRESH_LOW_P  -0.15f

/* ═══════ 事件映射表 (Flash 常量) ═══════ */
static const EmotionEventDef_t event_table[] = {
    {EVENT_TOUCH_TAP,      0.15f,  0.10f},
    {EVENT_TOUCH_DOUBLE,   0.05f,  0.20f},
    {EVENT_TOUCH_LONG,    -0.10f,  0.25f},
    {EVENT_TOUCH_SWIPE_H,  0.30f,  0.10f},
    {EVENT_TOUCH_SWIPE_V,  0.15f,  0.30f},
    {EVENT_TOUCH_CIRCLE,   0.20f,  0.20f},
    {EVENT_IDLE_30S,      -0.10f, -0.10f},
    {EVENT_IDLE_120S,     -0.20f, -0.30f},
    {EVENT_LLM_POSITIVE,   0.20f,  0.05f},
    {EVENT_LLM_NEGATIVE,  -0.20f,  0.10f},
    {EVENT_RANDOM_FLUCT,   0.00f,  0.00f}  /* 偏移量运行时随机 */
};

#define EVENT_TABLE_SIZE (sizeof(event_table)/sizeof(event_table[0]))

/* ═══════ 钳位辅助 ═══════ */
static float clamp_f(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ═══════ 表情判定 ═══════ */
static FaceID_t classify_face(float p, float a) {
    if (p >= THRESH_HIGH_P) {
        if (a > THRESH_HIGH_A)       return FACE_HAPPY;
        else if (a < THRESH_LOW_A)   return FACE_RELAXED;
        else                         return FACE_CONTENT;
    } else if (p <= THRESH_LOW_P) {
        if (a > THRESH_HIGH_A)       return FACE_ANGRY;
        else if (a < THRESH_LOW_A)   return FACE_DEPRESSED;
        else                         return FACE_SAD;
    } else {
        if (a >= THRESH_HIGH_A)       return FACE_SURPRISED;
        else if (a <= THRESH_LOW_A)   return FACE_BORED;
        else                         return FACE_NEUTRAL;
    }
}

/* ═══════ 公开函数 ═══════ */

void Emotion_Init(void) {
    emo_pleasure    = 0.0f;
    emo_arousal     = 0.0f;
    emo_face        = FACE_NEUTRAL;
}

void Emotion_GetState(float *p, float *a) {
    *p = emo_pleasure;
    *a = emo_arousal;
}

FaceID_t Emotion_GetFace(void) {
    return emo_face;
}

void Emotion_Event(EmotionEvent_t event, float intensity) {
    float p_delta = 0.0f;
    float a_delta = 0.0f;
    uint8_t i;

    /* 查表找事件定义 */
    for (i = 0; i < EVENT_TABLE_SIZE; i++) {
        if (event_table[i].event == event) {
            p_delta = event_table[i].p_delta;
            a_delta = event_table[i].a_delta;
            break;
        }
    }

    /* 随机波动特殊处理 */
    if (event == EVENT_RANDOM_FLUCT) {
        static uint32_t lcg = 12345;
        float r;
        lcg = lcg * 1103515245u + 12345u;
        r = ((int32_t)(lcg & 0x7FFF) - 16384) / 16384.0f;  /* -1.0 ~ +1.0 */
        p_delta = r * 0.10f;
        a_delta = r * 0.10f;
    }

    /* intensity 缩放 (默认 1.0) */
    p_delta *= intensity;
    a_delta *= intensity;

    emo_pleasure = clamp_f(emo_pleasure + p_delta, -1.0f, 1.0f);
    emo_arousal  = clamp_f(emo_arousal  + a_delta, -1.0f, 1.0f);

    /* 立即重新判定表情 (避免等待 100ms Emotion_Update 周期) */
    emo_face = classify_face(emo_pleasure, emo_arousal);
}

void Emotion_Update(void) {
    /* 指数衰减因子: P_decay = exp(-ln(2) / 600) ≈ 0.998845 */
    /*              A_decay = exp(-ln(2) / 400) ≈ 0.998268 */
    #define P_DECAY_PER_STEP  0.998845f
    #define A_DECAY_PER_STEP  0.998268f

    emo_pleasure *= P_DECAY_PER_STEP;
    emo_arousal  *= A_DECAY_PER_STEP;

    /* 钳位 */
    emo_pleasure = clamp_f(emo_pleasure, -1.0f, 1.0f);
    emo_arousal  = clamp_f(emo_arousal,  -1.0f, 1.0f);

    /* 更新表情 */
    emo_face = classify_face(emo_pleasure, emo_arousal);
}

void Emotion_InjectRaw(float p_delta, float a_delta) {
    emo_pleasure = clamp_f(emo_pleasure + p_delta, -1.0f, 1.0f);
    emo_arousal  = clamp_f(emo_arousal  + a_delta, -1.0f, 1.0f);
    emo_face = classify_face(emo_pleasure, emo_arousal);
}
