// config.h — Build constants extracted from inline defines
#ifndef EV_CONFIG_H
#define EV_CONFIG_H

#define FADE_SPEED_DEFAULT 2.0f
#define TRANSITION_HOLD_DEFAULT 0.15f
#define MAX_RAIN 200
#define MENU_MAX_ITEMS 8
#define PAUSE_ITEM_COUNT 3
#define SETTINGS_ITEM_COUNT 5

// Spring defaults (Kowalski)
#define SPRING_K 280.0f
#define SPRING_D 26.0f
#define SPRING_M 0.9f

// Z-fighting prevention — geometry offset layers
// Use these instead of magic 0.01f nudges.  Each layer guarantees no overlap.
#define Z_DECAL_BIAS   0.003f   // decal above parent surface (rugs, puddles)
#define Z_DECAL_BIAS2  0.006f   // second decal layer (borders on rugs, stains on carpets)
#define Z_TRIM_BIAS    0.005f   // trim/baseboard/molding offset from parent wall

// Hard cut flash (Blendo-style)
#define HARD_CUT_FLASH_DURATION 0.08f
#define HARD_CUT_FLASH_R 1.0f
#define HARD_CUT_FLASH_G 0.95f
#define HARD_CUT_FLASH_B 0.85f

#endif
