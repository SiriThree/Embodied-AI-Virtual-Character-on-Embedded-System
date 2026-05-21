#ifndef __GESTURE_H
#define __GESTURE_H

#include "stm32f10x.h"

typedef enum
{
    GESTURE_NONE = 0,
    GESTURE_TAP,
    GESTURE_LONG_PRESS,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_SWIPE_UP,
    GESTURE_SWIPE_DOWN
} GestureType;

void Gesture_Init(void);
GestureType Gesture_Update(void);

#endif