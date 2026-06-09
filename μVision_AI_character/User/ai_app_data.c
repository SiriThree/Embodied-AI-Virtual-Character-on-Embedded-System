/* Shared resources: scene metadata, UI strings and idle prompt tables. */
#include "ai_app_data.h"

SceneInfo g_scenes[MAX_SCENE_COUNT] =
{
    {TXT_SCENE_SHOPPING, TXT_SCENE_SHOPPING_INTRO, "shopping"},
    {TXT_SCENE_GAMING, TXT_SCENE_GAMING_INTRO, "gaming"},
    {TXT_SCENE_CAFE, TXT_SCENE_CAFE_INTRO, "cafe"},
    {TXT_SCENE_LIBRARY, TXT_SCENE_LIBRARY_INTRO, "library"},
    {TXT_SCENE_NIGHT, TXT_SCENE_NIGHT_INTRO, "night"},
    {TXT_SCENE_ENCOURAGE, TXT_SCENE_ENCOURAGE_INTRO, "encourage"},
    {TXT_SCENE_WALK, TXT_SCENE_WALK_INTRO, "walk"},
    {TXT_SCENE_DINNER, TXT_SCENE_DINNER_INTRO, "dinner"},
    {TXT_SCENE_MOVIE, TXT_SCENE_MOVIE_INTRO, "movie"},
    {TXT_SCENE_MUSIC, TXT_SCENE_MUSIC_INTRO, "music"},
    {TXT_SCENE_EXAM, TXT_SCENE_EXAM_INTRO, "exam"},
    {TXT_SCENE_REST, TXT_SCENE_REST_INTRO, "rest"}
};

const uint8_t g_scene_count = MAX_SCENE_COUNT;

IdleFeedback g_idle_feedbacks[4] =
{
    {TXT_IDLE_1},
    {TXT_IDLE_2},
    {TXT_IDLE_3},
    {TXT_IDLE_4}
};

const uint8_t g_idle_feedback_count = 4;

uint8_t global_volume = 12; 
uint8_t g_is_audio_playing = 0; 
uint32_t g_led_breathing_timer = 0;
uint8_t g_led_speed = 500;
