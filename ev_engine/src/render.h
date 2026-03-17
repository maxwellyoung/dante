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
    int exposureLoc;
    int grainLoc;
    int flashLoc;        // flash intensity (0-1), for scene cut flashes
    int flashColorLoc;   // flash color (vec3)
    bool ready;
} EVPostFX;

EVPostFX LoadEVPostFX(void);
void UnloadEVPostFX(EVPostFX *pfx);
void SetPostFXWarmth(EVPostFX *pfx, float warmth);
void SetPostFXExposure(EVPostFX *pfx, float exposure);
void SetPostFXGrain(EVPostFX *pfx, float grain);
void SetPostFXFlash(EVPostFX *pfx, float intensity, float r, float g, float b);

void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded,
                   Model *cyl_model, bool cyl_model_loaded,
                   Model *sphere_model, bool sphere_model_loaded,
                   Model *cone_model, bool cone_model_loaded,
                   Model *skytower_model, bool skytower_model_loaded,
                   bool indoor, float time);
void draw_hud(Player *player, Scene *scene);
void draw_title(void);
void draw_night_sky(float time);
void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target);
void draw_dust_motes(Camera3D camera, float time);
void draw_text_box(const char *text, int y, int font_size, Color text_color);

#endif
