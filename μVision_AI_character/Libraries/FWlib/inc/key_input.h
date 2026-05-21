#ifndef __KEY_INPUT_H
#define __KEY_INPUT_H

#include "stm32f10x.h"

typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_K1_SHORT,
    KEY_EVENT_K1_LONG,
    KEY_EVENT_K2_SHORT,
    KEY_EVENT_K2_LONG
} KeyEvent;

void KeyInput_Init(void);
KeyEvent KeyInput_Update(void);

#endif