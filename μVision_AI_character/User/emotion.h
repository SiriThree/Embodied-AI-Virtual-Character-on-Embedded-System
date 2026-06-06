#ifndef __EMOTION_H
#define __EMOTION_H
#include "stm32f10x.h"

/* 表情枚举 */
typedef enum {
    FACE_HAPPY      = 0,
    FACE_CONTENT    = 1,
    FACE_RELAXED    = 2,
    FACE_SURPRISED  = 3,
    FACE_NEUTRAL    = 4,
    FACE_BORED      = 5,
    FACE_ANGRY      = 6,
    FACE_SAD        = 7,
    FACE_DEPRESSED  = 8
} FaceID_t;

/* 情绪事件枚举 */
typedef enum {
    EVENT_TOUCH_TAP       = 0,
    EVENT_TOUCH_DOUBLE    = 1,
    EVENT_TOUCH_LONG      = 2,
    EVENT_TOUCH_SWIPE_H   = 3,
    EVENT_TOUCH_SWIPE_V   = 4,
    EVENT_TOUCH_CIRCLE    = 5,
    EVENT_IDLE_30S        = 6,
    EVENT_IDLE_120S       = 7,
    EVENT_LLM_POSITIVE    = 8,
    EVENT_LLM_NEGATIVE    = 9,
    EVENT_RANDOM_FLUCT    = 10
} EmotionEvent_t;

/* 情绪事件定义表 (Flash 常量区) */
typedef struct {
    EmotionEvent_t event;
    float          p_delta;
    float          a_delta;
} EmotionEventDef_t;

void     Emotion_Init(void);
void     Emotion_Event(EmotionEvent_t event, float intensity);
void     Emotion_Update(void);
void     Emotion_GetState(float *p, float *a);
FaceID_t Emotion_GetFace(void);

/* 原始情绪注入: 直接加减 P/A (上位机使用) */
void     Emotion_InjectRaw(float p_delta, float a_delta);
#endif
