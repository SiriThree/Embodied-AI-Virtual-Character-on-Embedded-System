AI Avatars 160x120 RGB565 Pack

Files:
- ai_avatars.h
- ai_avatars.c
- avatar_contact_sheet_160x120.png
- preview PNG files

Usage:
1. Replace your old ai_avatars.h/c with these new files.
2. Add ai_avatars.c to Keil project if needed.
3. In main.c, set avatar position approximately to center:
   #define AVATAR_X 40
   #define AVATAR_Y 8

Notes:
- Width = 160, Height = 120.
- One image = 38400 bytes.
- 7 images = 268800 bytes.
- This version is lighter than the 200x150 pack and should be more stable on STM32F103VET6.

State mapping:
- normal_idle -> serene_portrait_of_a_fantasy_warrior
- thinking    -> serene_elegance_in_flowing_teal_and_gold
- happy       -> cheerful_warrior_with_flowing_hair
- tired       -> sleepy_elegance_in_soft_fantasy_attire
- curious     -> elegant_anime_portrait_with_golden_accents
- gentle      -> serene_elegance_in_golden_light
- shy         -> shy_elegance_in_fantasy_attire
