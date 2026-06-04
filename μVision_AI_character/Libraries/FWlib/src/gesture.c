#include "gesture.h"
#include "bsp_xpt2046_lcd.h"
#include "./lcd/bsp_ili9341_lcd.h"

#define GESTURE_MAX_POINTS          32

#define GESTURE_SWIPE_THRESHOLD     55
#define GESTURE_TAP_MOVE_THRESHOLD  25
#define GESTURE_LONG_PRESS_COUNT    80

typedef struct
{
    int16_t x;
    int16_t y;
} GesturePoint;

static GesturePoint g_points[GESTURE_MAX_POINTS];
static uint16_t g_point_count = 0;

static uint8_t g_is_touching = 0;
static uint16_t g_press_count = 0;
static uint8_t g_last_point_valid = 0;
static uint16_t g_last_point_x = 0;
static uint16_t g_last_point_y = 0;

static void Gesture_Reset(void);
static void Gesture_AddPoint(int16_t x, int16_t y);
static GestureType Gesture_Analyze(void);


void Gesture_Init(void)
{
    Gesture_Reset();
}

uint8_t Gesture_GetLastPoint(uint16_t *x, uint16_t *y)
{
    if (!g_last_point_valid)
    {
        return 0;
    }

    *x = g_last_point_x;
    *y = g_last_point_y;
    return 1;
}


GestureType Gesture_Update(void)
{
    strType_XPT2046_Coordinate touch;

    /*
     * 当前有触摸按下
     */
    if (XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel)
    {
        g_is_touching = 1;
        g_press_count++;

        if (XPT2046_Get_TouchedPoint(&touch, strXPT2046_TouchPara))
        {
            Gesture_AddPoint(touch.x, touch.y);
        }

        return GESTURE_NONE;
    }

    /*
     * 当前没有触摸，但上一轮是触摸状态：
     * 说明刚刚松手，此时分析整段轨迹
     */
    if (g_is_touching)
    {
        GestureType result;

        g_is_touching = 0;

        result = Gesture_Analyze();

        Gesture_Reset();

        return result;
    }

    return GESTURE_NONE;
}


static void Gesture_Reset(void)
{
    uint16_t i;

    for (i = 0; i < GESTURE_MAX_POINTS; i++)
    {
        g_points[i].x = 0;
        g_points[i].y = 0;
    }

    g_point_count = 0;
    g_press_count = 0;
    g_is_touching = 0;
}


static void Gesture_AddPoint(int16_t x, int16_t y)
{
    if (g_point_count >= GESTURE_MAX_POINTS)
    {
        return;
    }

    /*
     * 简单去抖：如果和上一个点太近，就不重复记录
     */
    if (g_point_count > 0)
    {
        int16_t dx = x - g_points[g_point_count - 1].x;
        int16_t dy = y - g_points[g_point_count - 1].y;

        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;

        if (dx < 3 && dy < 3)
        {
            return;
        }
    }

    g_points[g_point_count].x = x;
    g_points[g_point_count].y = y;
    g_point_count++;

    g_last_point_x = (uint16_t)x;
    g_last_point_y = (uint16_t)y;
    g_last_point_valid = 1;
}


static GestureType Gesture_Analyze(void)
{
    int16_t start_x, start_y;
    int16_t end_x, end_y;
    int16_t dx, dy;
    int16_t abs_dx, abs_dy;

    if (g_point_count < 2)
    {
        if (g_press_count > GESTURE_LONG_PRESS_COUNT)
        {
            return GESTURE_LONG_PRESS;
        }

        return GESTURE_TAP;
    }

    start_x = g_points[0].x;
    start_y = g_points[0].y;

    end_x = g_points[g_point_count - 1].x;
    end_y = g_points[g_point_count - 1].y;

    dx = end_x - start_x;
    dy = end_y - start_y;

    abs_dx = dx >= 0 ? dx : -dx;
    abs_dy = dy >= 0 ? dy : -dy;

    /*
     * 移动很小：点击或长按
     */
    if (abs_dx < GESTURE_TAP_MOVE_THRESHOLD &&
        abs_dy < GESTURE_TAP_MOVE_THRESHOLD)
    {
        if (g_press_count > GESTURE_LONG_PRESS_COUNT)
        {
            return GESTURE_LONG_PRESS;
        }

        return GESTURE_TAP;
    }

    /*
     * 横向滑动
     */
    if (abs_dx > GESTURE_SWIPE_THRESHOLD &&
        abs_dx > abs_dy * 2)
    {
        if (dx > 0)
        {
            return GESTURE_SWIPE_RIGHT;
        }
        else
        {
            return GESTURE_SWIPE_LEFT;
        }
    }

    /*
     * 纵向滑动
     */
    if (abs_dy > GESTURE_SWIPE_THRESHOLD &&
        abs_dy > abs_dx * 2)
    {
        if (dy > 0)
        {
            return GESTURE_SWIPE_DOWN;
        }
        else
        {
            return GESTURE_SWIPE_UP;
        }
    }

    return GESTURE_NONE;
}
