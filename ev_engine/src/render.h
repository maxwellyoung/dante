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
    int flashLoc;
    int flashColorLoc;
    int saturationLoc;
    int caAmountLoc;
    int contrastLoc;
    int vignetteLoc;
    int tintLoc;
    // Dramatic effect uniforms
    int ditherLoc;       // ordered dithering intensity
    int scanlineLoc;     // CRT scanline intensity
    int bloomLoc;        // glow/bloom intensity
    int posterizeLoc;    // color quantization levels (0=off, 4-32)
    int pixelateLoc;     // pixel size multiplier (1=off, 2-4=chunky)
    int sharpenLoc;      // edge sharpening intensity
    int speedLoc;        // player speed (0-1 normalized for effects)
    bool ready;
} EVPostFX;

// Visual style presets — Shift+number keys
typedef struct {
    const char *name;
    float saturation;    // 0=mono, 1=full color
    float caAmount;      // chromatic aberration
    float contrast;      // S-curve intensity
    float vignette;      // edge darkening
    float grain;         // film grain
    float exposure_bias; // added to scene exposure
    float tint[3];       // RGB color grade
    // Dramatic effects
    float dither;        // ordered dithering (0=off, 1=full)
    float scanline;      // CRT scanlines (0=off, 1=full)
    float bloom;         // glow bleeding (0=off, 1=heavy)
    float posterize;     // color levels (0=off, 8/16/32)
    float pixelate;      // pixel size (1=off, 2-4=chunky)
    float sharpen;       // edge sharpening (0=off, 1=heavy)
} VisualStyle;

#define STYLE_COUNT 9
extern const VisualStyle visual_styles[STYLE_COUNT];

EVPostFX LoadEVPostFX(void);
void UnloadEVPostFX(EVPostFX *pfx);
void SetPostFXWarmth(EVPostFX *pfx, float warmth);
void SetPostFXExposure(EVPostFX *pfx, float exposure);
void SetPostFXGrain(EVPostFX *pfx, float grain);
void SetPostFXFlash(EVPostFX *pfx, float intensity, float r, float g, float b);
void SetPostFXSaturation(EVPostFX *pfx, float saturation);
void SetPostFXCA(EVPostFX *pfx, float caAmount);
void SetPostFXContrast(EVPostFX *pfx, float contrast);
void SetPostFXVignette(EVPostFX *pfx, float vignette);
void SetPostFXTint(EVPostFX *pfx, float r, float g, float b);
void ApplyVisualStyle(EVPostFX *pfx, int style_index);
void SetPostFXSpeed(EVPostFX *pfx, float speed);

void draw_shadow_pass(Scene *scene, EVLighting *lighting,
                      Model *cube_model, Model *cyl_model,
                      Model *sphere_model, Model *cone_model);
void draw_earth(Camera3D camera, float time,
                Model *sphere_model, EVLighting *lighting,
                Vector3 earth_center);
void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded,
                   Model *cyl_model, bool cyl_model_loaded,
                   Model *sphere_model, bool sphere_model_loaded,
                   Model *cone_model, bool cone_model_loaded,
                   Model *skytower_model, bool skytower_model_loaded,
                   bool indoor, float time);
void draw_hud(Player *player, Scene *scene);
void draw_title(void);
void reset_title_state(void);
void draw_night_sky(float time);
void draw_dawn_sky(float time);
void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target);
void draw_dust_motes(Camera3D camera, float time);
void draw_zero_g_sparkles(Camera3D camera, float time);
void draw_rain(Camera3D camera, float time);
void draw_text_box(const char *text, int y, int font_size, Color text_color);

#endif
