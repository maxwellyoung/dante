// render.h — Rendering pipeline with post-processing
#ifndef EV_RENDER_H
#define EV_RENDER_H

#include "ev_types.h"
#include "lighting.h"

typedef struct {
    Shader postfx;
    int timeLoc;
    int resolutionLoc;
    bool ready;
} EVPostFX;

EVPostFX LoadEVPostFX(void);
void UnloadEVPostFX(EVPostFX *pfx);

void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded);
void draw_dither_overlay(void);
void draw_hud(Player *player, Scene *scene, GameState state, int tasks_done, int total_tasks,
              const char *screen_text, float screen_text_timer);
void draw_title(float planet_angle);
void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target);

#endif
