// palette.h — Design system color palette for Endearing Void
// Change these to retheme the entire game.
#ifndef EV_PALETTE_H
#define EV_PALETTE_H

#include "raylib.h"

// === CORE PALETTE ===

// Neutrals
#define PAL_BLACK     (Color){12, 12, 14, 255}
#define PAL_CHARCOAL  (Color){35, 33, 30, 255}
#define PAL_DARK      (Color){55, 52, 48, 255}
#define PAL_MID       (Color){130, 126, 120, 255}
#define PAL_LIGHT     (Color){210, 206, 200, 255}
#define PAL_CREAM     (Color){232, 228, 222, 255}
#define PAL_WHITE     (Color){245, 242, 238, 255}

// Materials
#define PAL_CONCRETE  (Color){158, 155, 150, 255}
#define PAL_BRASS     (Color){178, 155, 107, 255}
#define PAL_WOOD      (Color){140, 105, 65, 255}
#define PAL_WOOD_DARK (Color){95, 70, 42, 255}
#define PAL_MARBLE_A  (Color){195, 192, 186, 255}
#define PAL_MARBLE_B  (Color){180, 177, 172, 255}
#define PAL_LEATHER   (Color){110, 75, 45, 255}

// Accents — bold, readable at 480x300
#define PAL_RED       (Color){195, 45, 40, 255}
#define PAL_BLUE      (Color){40, 65, 160, 255}
#define PAL_GREEN     (Color){45, 100, 65, 255}
#define PAL_GOLD      (Color){210, 180, 100, 255}
#define PAL_TERRACOTTA (Color){175, 75, 55, 255}
#define PAL_NAVY      (Color){30, 40, 75, 255}

// Lights
#define PAL_LIGHT_WARM  (Color){240, 220, 175, 180}
#define PAL_LIGHT_COOL  (Color){180, 200, 225, 150}
#define PAL_GLOW_AMBER  (Color){240, 195, 90, 120}

// Text
#define PAL_TEXT        (Color){232, 228, 222, 240}
#define PAL_TEXT_DIM    (Color){165, 160, 152, 180}
#define PAL_TEXT_SHADOW (Color){0, 0, 0, 180}

// Scene fog colors — tinted per room
#define PAL_FOG_GREEN   (Color){50, 72, 58, 255}
#define PAL_FOG_RED     (Color){120, 70, 58, 255}
#define PAL_FOG_GOLD    (Color){155, 138, 100, 255}
#define PAL_FOG_CONCRETE (Color){125, 123, 120, 255}
#define PAL_FOG_NIGHT   (Color){18, 22, 38, 255}
#define PAL_FOG_BRASS   (Color){145, 130, 100, 255}
#define PAL_FOG_VOID    (Color){4, 5, 12, 255}
#define PAL_FOG_STATION (Color){8, 10, 20, 255}

// Space hotel materials — must read at 480x300, not submarine-dark
#define PAL_HULL        (Color){105, 112, 125, 255}
#define PAL_HULL_LIGHT  (Color){135, 142, 155, 255}
#define PAL_PORTHOLE    (Color){4, 6, 14, 255}
#define PAL_EARTH_GLOW  (Color){60, 130, 200, 140}
#define PAL_STAR_WHITE  (Color){240, 238, 232, 200}

#endif
