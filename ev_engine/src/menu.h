// menu.h — Pause menu and settings overlay
#ifndef EV_MENU_H
#define EV_MENU_H

#include "game_ctx.h"

void menu_open(GameCtx *g, MenuMode mode);
void menu_close(GameCtx *g);
void update_menu_springs(GameCtx *g, float dt);
bool update_pause_menu(GameCtx *g);
void draw_pause_menu(GameCtx *g);

#endif // EV_MENU_H
