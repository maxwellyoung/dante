// ui.h — HUD, crosshair, compass, interaction prompts
// Spring physics + Susan Kare pixel icons
// "No hand-holding" — the UI whispers, never shouts.
#ifndef EV_UI_H
#define EV_UI_H

#include "ev_types.h"

// Initialize UI state (call once at startup)
void ui_init(void);

// Reset UI springs (call on scene change)
void ui_reset(void);

// Main HUD draw — crosshair, compass, interaction prompts
// Call after 3D scene, before post-FX
void ui_draw_hud(Player *player, Scene *scene);

// Compass only — subtle axis indicator around crosshair
// Shows N/S/E/W ticks based on camera yaw
void ui_draw_compass(Player *player);

// Interaction prompt — spring-animated ring + icon + key
// Called internally by ui_draw_hud when targeting an object
void ui_draw_interaction(Player *player, Scene *scene);

// Crosshair — thin cross with spring scale
void ui_draw_crosshair(float target_scale);

// Pixel icons — Susan Kare 5x5/7x7 symbols
void ui_draw_pixel_icon(int cx, int cy, const char *name, unsigned char alpha);

#endif
