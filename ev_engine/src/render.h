// render.h — Rendering pipeline with post-processing
#ifndef EV_RENDER_H
#define EV_RENDER_H

#include "ev_types.h"
#include "lighting.h"

typedef struct {
    Shader postfx;
    int timeLoc;
    int resolutionLoc;
    int warmthLoc;
    bool ready;
} EVPostFX;

EVPostFX LoadEVPostFX(void);
void UnloadEVPostFX(EVPostFX *pfx);
void SetPostFXWarmth(EVPostFX *pfx, float warmth);

void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded);
void draw_hud(Player *player, Scene *scene);
void draw_title(float planet_angle);
void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target);

#endif
