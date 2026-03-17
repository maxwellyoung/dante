// render.c — Rendering pipeline with post-processing
// No hand-holding. No task counter. No exit beacon.
// The space speaks for itself.
#include "render.h"
#include "palette.h"
#include "raymath.h"
#include "rlgl.h"
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>     // glPolygonOffset, glEnable — decal z-fighting fix
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>

// ============================================================
// POST-PROCESSING SHADER
// Architectural photograph + 16mm film stock
// warmth: color temperature (narrative progression)
// exposure: global brightness (time of day / scene mood)
// grain: film grain intensity (lo-fi 16mm Godard stock)
// flash/flashColor: scene-cut flash (Blendo smash cuts)
// Chromatic aberration at edges (lo-fi lens)
// ============================================================

static const char *postfx_vs =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *postfx_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0;\n"
    "uniform float time;\n"
    "uniform vec2 resolution;\n"
    "uniform float warmth;\n"
    "uniform float exposure;\n"
    "uniform float grain;\n"
    "uniform float flash;\n"
    "uniform vec3 flashColor;\n"
    "uniform float saturation;\n"
    "uniform float caAmount;\n"
    "uniform float contrast;\n"
    "uniform float vignetteAmt;\n"
    "uniform vec3 tint;\n"
    "uniform float ditherAmt;\n"
    "uniform float scanlineAmt;\n"
    "uniform float bloomAmt;\n"
    "uniform float posterizeAmt;\n"
    "uniform float pixelateAmt;\n"
    "uniform float sharpenAmt;\n"
    "uniform float speedNorm;\n"    // 0-1 normalized player speed
    "out vec4 finalColor;\n"
    "\n"
    "float grainHash(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.x + p3.y) * p3.z);\n"
    "}\n"
    "\n"
    // 4x4 Bayer matrix for ordered dithering
    "float bayer4(vec2 p) {\n"
    "    int x = int(mod(p.x, 4.0));\n"
    "    int y = int(mod(p.y, 4.0));\n"
    "    int idx = x + y * 4;\n"
    "    float m[16] = float[16](\n"
    "         0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,\n"
    "        12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,\n"
    "         3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,\n"
    "        15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0\n"
    "    );\n"
    "    return m[idx];\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 px = 1.0 / resolution;\n"
    "\n"
    "    // --- Pixelation — chunky pixels ---\n"
    "    if (pixelateAmt > 1.0) {\n"
    "        vec2 pixSize = px * pixelateAmt;\n"
    "        uv = floor(uv / pixSize) * pixSize + pixSize * 0.5;\n"
    "    }\n"
    "\n"
    "    vec2 fromCenter = uv - 0.5;\n"
    "\n"
    "    // --- Barrel distortion — subtle lo-fi lens warp ---\n"
    "    // Edges of the 480x300 frame bulge slightly inward.\n"
    "    // Increases with speed for a visceral movement feel.\n"
    "    {\n"
    "        float r2 = dot(fromCenter, fromCenter);\n"
    "        float distort = 1.0 + r2 * (0.03 + speedNorm * 0.04);\n"
    "        uv = 0.5 + fromCenter * distort;\n"
    "        fromCenter = uv - 0.5;\n"
    "    }\n"
    "\n"
    "    // --- Chromatic aberration — RGB split at edges ---\n"
    "    float caStrength = dot(fromCenter, fromCenter) * caAmount;\n"
    "    vec2 caOffset = fromCenter * caStrength * px * 3.0;\n"
    "    float r = texture(texture0, uv + caOffset).r;\n"
    "    float g = texture(texture0, uv).g;\n"
    "    float b = texture(texture0, uv - caOffset).b;\n"
    "    vec3 col = vec3(r, g, b);\n"
    "\n"
    "    // --- Sharpening — unsharp mask ---\n"
    "    if (sharpenAmt > 0.0) {\n"
    "        vec3 blur = vec3(0.0);\n"
    "        blur += texture(texture0, uv + vec2(-1, 0) * px).rgb;\n"
    "        blur += texture(texture0, uv + vec2( 1, 0) * px).rgb;\n"
    "        blur += texture(texture0, uv + vec2( 0,-1) * px).rgb;\n"
    "        blur += texture(texture0, uv + vec2( 0, 1) * px).rgb;\n"
    "        blur *= 0.25;\n"
    "        col += (col - blur) * sharpenAmt * 2.0;\n"
    "    }\n"
    "\n"
    "    // --- Bloom — bright areas bleed light ---\n"
    "    if (bloomAmt > 0.0) {\n"
    "        vec3 bloomCol = vec3(0.0);\n"
    "        float bw = 3.0; // bloom kernel width\n"
    "        for (int bx = -2; bx <= 2; bx++) {\n"
    "            for (int by = -2; by <= 2; by++) {\n"
    "                vec3 s = texture(texture0, uv + vec2(float(bx), float(by)) * px * bw).rgb;\n"
    "                float sl = dot(s, vec3(0.299, 0.587, 0.114));\n"
    "                bloomCol += s * smoothstep(0.5, 1.0, sl);\n"
    "            }\n"
    "        }\n"
    "        bloomCol /= 25.0;\n"
    "        col += bloomCol * bloomAmt * 1.5;\n"
    "    }\n"
    "\n"
    "    // --- SSAO — screen-space edge darkening ---\n"
    "    float center_luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    float ao_accum = 0.0;\n"
    "    for (int i = 0; i < 8; i++) {\n"
    "        float angle = float(i) * 0.785398;\n"
    "        vec2 offset = vec2(cos(angle), sin(angle)) * px * 2.5;\n"
    "        float neighbor_luma = dot(texture(texture0, uv + offset).rgb, vec3(0.299, 0.587, 0.114));\n"
    "        ao_accum += max(0.0, center_luma - neighbor_luma);\n"
    "    }\n"
    "    float ssao = 1.0 - ao_accum * 0.45;\n"  // stronger AO
    "    col *= max(ssao, 0.4);\n"
    "\n"
    "    // --- Exposure ---\n"
    "    col *= exp2(exposure);\n"
    "\n"
    "    // --- Saturation ---\n"
    "    float luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    col = mix(vec3(luma), col, saturation + warmth * 0.08);\n"
    "\n"
    "    // --- Color grading — warmth shift + tint ---\n"
    "    col = pow(max(col, vec3(0.0)), vec3(0.97 + warmth*0.03, 1.0, 1.02 - warmth*0.05));\n"
    "    col = col * 0.97 + vec3(0.02, 0.018, 0.015);\n"
    "    col *= tint;\n"
    "\n"
    "    // --- Contrast S-curve ---\n"
    "    vec3 curved = smoothstep(0.0, 1.0, col * 1.05 - 0.02);\n"
    "    col = mix(col, curved, contrast);\n"
    "\n"
    "    // --- Posterization — color quantization ---\n"
    "    if (posterizeAmt > 1.0) {\n"
    "        col = floor(col * posterizeAmt + 0.5) / posterizeAmt;\n"
    "    }\n"
    "\n"
    "    // --- Ordered dithering — Bayer 4x4 ---\n"
    "    if (ditherAmt > 0.0) {\n"
    "        vec2 ditherCoord = gl_FragCoord.xy;\n"
    "        float d = bayer4(ditherCoord) - 0.5;\n"
    "        col += d * ditherAmt * 0.15;\n"
    "        // Dither-based color reduction for retro look\n"
    "        if (ditherAmt > 0.5) {\n"
    "            float levels = mix(32.0, 6.0, (ditherAmt - 0.5) * 2.0);\n"
    "            col = floor(col * levels + d * 0.8) / levels;\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // --- Film grain ---\n"
    "    if (grain > 0.0) {\n"
    "        float n = grainHash(uv * resolution + fract(time * 7.3) * 100.0);\n"
    "        n = (n - 0.5) * grain * 0.15;\n"  // stronger grain
    "        float shadow = 1.0 - smoothstep(0.0, 0.4, luma);\n"
    "        col += n * (0.6 + shadow * 0.6);\n"
    "    }\n"
    "\n"
    "    // --- CRT Scanlines ---\n"
    "    if (scanlineAmt > 0.0) {\n"
    "        float scanline = sin(gl_FragCoord.y * 3.14159) * 0.5 + 0.5;\n"
    "        scanline = pow(scanline, 1.5);\n"
    "        col *= mix(1.0, scanline, scanlineAmt * 0.4);\n"
    "        // Slight horizontal blur for CRT phosphor bleed\n"
    "        if (scanlineAmt > 0.5) {\n"
    "            vec3 left = texture(texture0, uv - vec2(px.x, 0)).rgb;\n"
    "            vec3 right = texture(texture0, uv + vec2(px.x, 0)).rgb;\n"
    "            col = mix(col, (col + left + right) / 3.0, (scanlineAmt - 0.5) * 0.4);\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // --- Vignette ---\n"
    "    float vig = 1.0 - dot((uv - 0.5) * 0.9, (uv - 0.5) * 0.9);\n"
    "    vig = clamp(pow(vig, 0.7), 0.0, 1.0);\n"
    "    float vigDark = mix(0.75, 1.0, vig);\n"  // darker vignette
    "    col *= mix(1.0, vigDark, vignetteAmt);\n"
    "\n"
    "    // --- Speed lines — radial blur from center at high speed ---\n"
    "    if (speedNorm > 0.0) {\n"
    "        float edgeDist = length(fromCenter) * 2.0;\n"  // 0 at center, ~1 at edges
    "        float speedBlur = speedNorm * edgeDist * edgeDist;\n"  // stronger at edges
    "        if (speedBlur > 0.01 && edgeDist > 0.001) {\n"
    "            vec2 blurDir = (fromCenter / edgeDist) * speedBlur * 0.04;\n"
    "            vec3 blur = vec3(0.0);\n"
    "            for (int s = 1; s <= 4; s++) {\n"
    "                blur += texture(texture0, uv - blurDir * float(s)).rgb;\n"
    "            }\n"
    "            blur *= 0.25;\n"
    "            col = mix(col, blur, speedBlur * 0.5);\n"
    "        }\n"
    "        // Speed-reactive CA boost — edges split more at speed\n"
    "        float speedCA = speedNorm * edgeDist * 3.0;\n"
    "        if (speedCA > 0.1) {\n"
    "            vec2 scaOff = fromCenter * speedCA * px * 2.0;\n"
    "            col.r = mix(col.r, texture(texture0, uv + scaOff).r, speedNorm * 0.3);\n"
    "            col.b = mix(col.b, texture(texture0, uv - scaOff).b, speedNorm * 0.3);\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // --- Scene-cut flash ---\n"
    "    if (flash > 0.0) {\n"
    "        col = mix(col, flashColor, flash);\n"
    "    }\n"
    "\n"
    "    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0);\n"
    "}\n";

EVPostFX LoadEVPostFX(void) {
    EVPostFX pfx = {0};
    pfx.postfx = LoadShaderFromMemory(postfx_vs, postfx_fs);
    if (pfx.postfx.id > 0) {
        pfx.timeLoc = GetShaderLocation(pfx.postfx, "time");
        pfx.resolutionLoc = GetShaderLocation(pfx.postfx, "resolution");
        pfx.warmthLoc = GetShaderLocation(pfx.postfx, "warmth");
        pfx.exposureLoc = GetShaderLocation(pfx.postfx, "exposure");
        pfx.grainLoc = GetShaderLocation(pfx.postfx, "grain");
        pfx.flashLoc = GetShaderLocation(pfx.postfx, "flash");
        pfx.flashColorLoc = GetShaderLocation(pfx.postfx, "flashColor");
        pfx.saturationLoc = GetShaderLocation(pfx.postfx, "saturation");
        pfx.caAmountLoc = GetShaderLocation(pfx.postfx, "caAmount");
        pfx.contrastLoc = GetShaderLocation(pfx.postfx, "contrast");
        pfx.vignetteLoc = GetShaderLocation(pfx.postfx, "vignetteAmt");
        pfx.tintLoc = GetShaderLocation(pfx.postfx, "tint");
        pfx.ditherLoc = GetShaderLocation(pfx.postfx, "ditherAmt");
        pfx.scanlineLoc = GetShaderLocation(pfx.postfx, "scanlineAmt");
        pfx.bloomLoc = GetShaderLocation(pfx.postfx, "bloomAmt");
        pfx.posterizeLoc = GetShaderLocation(pfx.postfx, "posterizeAmt");
        pfx.pixelateLoc = GetShaderLocation(pfx.postfx, "pixelateAmt");
        pfx.sharpenLoc = GetShaderLocation(pfx.postfx, "sharpenAmt");
        pfx.speedLoc = GetShaderLocation(pfx.postfx, "speedNorm");

        float res[2] = {(float)RENDER_W, (float)RENDER_H};
        SetShaderValue(pfx.postfx, pfx.resolutionLoc, res, SHADER_UNIFORM_VEC2);

        // Defaults
        float zero = 0.0f;
        float one = 1.0f;
        SetShaderValue(pfx.postfx, pfx.warmthLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.exposureLoc, &zero, SHADER_UNIFORM_FLOAT);
        float defaultGrain = 0.5f;
        SetShaderValue(pfx.postfx, pfx.grainLoc, &defaultGrain, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.flashLoc, &zero, SHADER_UNIFORM_FLOAT);
        float white[3] = {1.0f, 1.0f, 1.0f};
        SetShaderValue(pfx.postfx, pfx.flashColorLoc, white, SHADER_UNIFORM_VEC3);
        float defaultSat = 0.92f;
        SetShaderValue(pfx.postfx, pfx.saturationLoc, &defaultSat, SHADER_UNIFORM_FLOAT);
        float defaultCA = 2.5f;
        SetShaderValue(pfx.postfx, pfx.caAmountLoc, &defaultCA, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.contrastLoc, &one, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.vignetteLoc, &one, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.tintLoc, white, SHADER_UNIFORM_VEC3);
        SetShaderValue(pfx.postfx, pfx.ditherLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.scanlineLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.bloomLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.posterizeLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.pixelateLoc, &one, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.sharpenLoc, &zero, SHADER_UNIFORM_FLOAT);

        pfx.ready = true;
        printf("[EV] Post-FX loaded — dither, scanlines, bloom, posterize, pixelate\n");
    } else {
        printf("[EV] WARNING: Post-processing shader failed\n");
        pfx.ready = false;
    }
    return pfx;
}

void UnloadEVPostFX(EVPostFX *pfx) {
    if (pfx->ready) {
        UnloadShader(pfx->postfx);
        pfx->ready = false;
    }
}

void SetPostFXWarmth(EVPostFX *pfx, float warmth) {
    if (pfx->ready) {
        SetShaderValue(pfx->postfx, pfx->warmthLoc, &warmth, SHADER_UNIFORM_FLOAT);
    }
}

void SetPostFXExposure(EVPostFX *pfx, float exposure) {
    if (pfx->ready) {
        SetShaderValue(pfx->postfx, pfx->exposureLoc, &exposure, SHADER_UNIFORM_FLOAT);
    }
}

void SetPostFXGrain(EVPostFX *pfx, float grain) {
    if (pfx->ready) {
        SetShaderValue(pfx->postfx, pfx->grainLoc, &grain, SHADER_UNIFORM_FLOAT);
    }
}

void SetPostFXFlash(EVPostFX *pfx, float intensity, float r, float g, float b) {
    if (pfx->ready) {
        SetShaderValue(pfx->postfx, pfx->flashLoc, &intensity, SHADER_UNIFORM_FLOAT);
        float col[3] = {r, g, b};
        SetShaderValue(pfx->postfx, pfx->flashColorLoc, col, SHADER_UNIFORM_VEC3);
    }
}

void SetPostFXSaturation(EVPostFX *pfx, float saturation) {
    if (pfx->ready) SetShaderValue(pfx->postfx, pfx->saturationLoc, &saturation, SHADER_UNIFORM_FLOAT);
}

void SetPostFXCA(EVPostFX *pfx, float caAmount) {
    if (pfx->ready) SetShaderValue(pfx->postfx, pfx->caAmountLoc, &caAmount, SHADER_UNIFORM_FLOAT);
}

void SetPostFXContrast(EVPostFX *pfx, float contrast) {
    if (pfx->ready) SetShaderValue(pfx->postfx, pfx->contrastLoc, &contrast, SHADER_UNIFORM_FLOAT);
}

void SetPostFXVignette(EVPostFX *pfx, float vignette) {
    if (pfx->ready) SetShaderValue(pfx->postfx, pfx->vignetteLoc, &vignette, SHADER_UNIFORM_FLOAT);
}

void SetPostFXTint(EVPostFX *pfx, float r, float g, float b) {
    if (pfx->ready) {
        float col[3] = {r, g, b};
        SetShaderValue(pfx->postfx, pfx->tintLoc, col, SHADER_UNIFORM_VEC3);
    }
}

void SetPostFXSpeed(EVPostFX *pfx, float speed) {
    if (pfx->ready)
        SetShaderValue(pfx->postfx, pfx->speedLoc, &speed, SHADER_UNIFORM_FLOAT);
}

// ============================================================
// VISUAL STYLE PRESETS
// Shift+1 through Shift+9 — each is a DRAMATICALLY different look
// ============================================================
//                                  sat    ca   con  vig  grn  exp   tint_r/g/b        dith scan blm  post pix  shrp
const VisualStyle visual_styles[STYLE_COUNT] = {
    // 1: Default — 16mm Godard. Warm, grainy, slight bloom.
    {"16mm Film",     0.92f, 2.5f, 1.0f, 1.2f, 0.6f,  0.0f, {1.0f,1.0f,1.0f},       0.0f,0.0f,0.3f, 0,  1,  0.0f},

    // 2: PS1 — ordered dithering, color quantization, chunky pixels
    {"PS1",           0.85f, 1.0f, 0.8f, 0.5f, 0.1f,  0.05f,{1.0f,0.98f,0.95f},      1.0f,0.0f,0.0f, 12, 2,  0.0f},

    // 3: Noir — crushed blacks, nearly mono, heavy vignette, sharp
    {"Noir",          0.15f, 1.5f, 1.8f, 2.5f, 0.8f, -0.2f, {0.92f,0.92f,1.0f},      0.0f,0.0f,0.0f, 0,  1,  1.5f},

    // 4: CRT — scanlines, bloom, phosphor glow, slight curve
    {"CRT",           0.95f, 4.0f, 1.1f, 1.5f, 0.3f,  0.05f,{1.0f,0.95f,0.9f},       0.0f,1.0f,0.6f, 0,  1,  0.0f},

    // 5: Godard — SATURATED, contrasty, red push, French New Wave
    {"Godard",        1.3f,  4.0f, 1.4f, 1.3f, 0.7f,  0.1f, {1.15f,0.95f,0.85f},     0.0f,0.0f,0.4f, 0,  1,  0.5f},

    // 6: VHS — heavy grain, CA blowout, warm mud, scanlines
    {"VHS",           0.7f,  8.0f, 0.6f, 1.8f, 1.2f, -0.1f, {1.1f,0.95f,0.85f},      0.2f,0.6f,0.2f, 0,  1,  0.0f},

    // 7: Neon — oversaturated, bloom heavy, high exposure, teal-orange
    {"Neon",          1.4f,  3.0f, 1.2f, 0.8f, 0.2f,  0.15f,{1.1f,0.9f,1.15f},       0.0f,0.0f,1.0f, 0,  1,  0.3f},

    // 8: Woodcut — extreme dithering, near-mono, posterized, high contrast
    {"Woodcut",       0.1f,  0.5f, 2.0f, 1.5f, 0.0f,  0.0f, {1.0f,0.98f,0.95f},      1.5f,0.0f,0.0f, 4,  1,  2.0f},

    // 9: Raw — nothing. Naked geometry and lighting.
    {"Raw",           1.0f,  0.0f, 0.0f, 0.0f, 0.0f,  0.0f, {1.0f,1.0f,1.0f},        0.0f,0.0f,0.0f, 0,  1,  0.0f},
};

static void set_pfx_float(EVPostFX *pfx, int loc, float val) {
    if (pfx->ready) SetShaderValue(pfx->postfx, loc, &val, SHADER_UNIFORM_FLOAT);
}

void ApplyVisualStyle(EVPostFX *pfx, int style_index) {
    if (style_index < 0 || style_index >= STYLE_COUNT) return;
    const VisualStyle *s = &visual_styles[style_index];
    SetPostFXSaturation(pfx, s->saturation);
    SetPostFXCA(pfx, s->caAmount);
    SetPostFXContrast(pfx, s->contrast);
    SetPostFXVignette(pfx, s->vignette);
    SetPostFXGrain(pfx, s->grain);
    SetPostFXTint(pfx, s->tint[0], s->tint[1], s->tint[2]);
    set_pfx_float(pfx, pfx->ditherLoc, s->dither);
    set_pfx_float(pfx, pfx->scanlineLoc, s->scanline);
    set_pfx_float(pfx, pfx->bloomLoc, s->bloom);
    set_pfx_float(pfx, pfx->posterizeLoc, s->posterize);
    set_pfx_float(pfx, pfx->pixelateLoc, s->pixelate);
    set_pfx_float(pfx, pfx->sharpenLoc, s->sharpen);
}

void draw_text_box(const char *text, int y, int font_size, Color text_color) {
    if (!text) return;
    if (font_size < 12) font_size = 12;
    int tw = MeasureText(text, font_size);
    int tx = RENDER_W / 2 - tw / 2;
    int pad = 12;
    // Fully opaque dark bar — readable on ANY background
    DrawRectangle(0, y - pad, RENDER_W, font_size + pad * 2,
                  (Color){8, 8, 10, 245});
    // Shadow — fully opaque, +1 offset
    DrawText(text, tx + 1, y + 1, font_size, (Color){0, 0, 0, 255});
    // Text — pure warm white
    DrawText(text, tx, y, font_size, text_color);
}

void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded,
                   Model *cyl_model, bool cyl_model_loaded,
                   Model *sphere_model, bool sphere_model_loaded,
                   Model *cone_model, bool cone_model_loaded,
                   Model *skytower_model, bool skytower_model_loaded,
                   bool indoor, float time) {
    if (lighting->ready) {
        UpdateEVLighting(lighting, player->camera, scene->fog_color, scene->fog_density);
    }

    BeginMode3D(player->camera);

    // Draw walls — two passes: solids first, then decals with polygon offset
    // This prevents z-fighting on overlay geometry (rugs, floor details, trim)
    bool has_decals = false;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            if (!has_decals) break;
            // Enable polygon offset for decal pass — biases depth to prevent z-fighting
            rlDrawRenderBatchActive();  // flush before GL state change
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
        }
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            // Pass 0: skip decals. Pass 1: only decals.
            if (pass == 0 && w->is_decal) { has_decals = true; continue; }
            if (pass == 1 && !w->is_decal) continue;

            if (lighting->ready) {
                SetMaterialId(lighting, (int)w->material);
                switch (w->shape) {
                    case SHAPE_CYLINDER:
                        if (cyl_model_loaded) {
                            cyl_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            DrawModelEx(*cyl_model, w->pos, (Vector3){0,1,0}, w->rotation_y,
                                       (Vector3){w->size.x, w->size.y, w->size.x}, WHITE);
                        }
                        break;
                    case SHAPE_SPHERE:
                        if (sphere_model_loaded) {
                            sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            DrawModelEx(*sphere_model, w->pos, (Vector3){0,1,0}, 0,
                                       (Vector3){w->size.x, w->size.y, w->size.z}, WHITE);
                        }
                        break;
                    case SHAPE_CONE:
                        if (cone_model_loaded) {
                            cone_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            DrawModelEx(*cone_model, w->pos, (Vector3){0,1,0}, w->rotation_y,
                                       (Vector3){w->size.x, w->size.y, w->size.z}, WHITE);
                        }
                        break;
                    case SHAPE_SKYTOWER:
                        if (skytower_model_loaded) {
                            skytower_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            // Model is Z-up from Blender — rotate -90° on X to stand upright
                            DrawModelEx(*skytower_model, w->pos, (Vector3){1,0,0}, -90,
                                       (Vector3){w->size.x, w->size.x, w->size.x}, WHITE);
                        }
                        break;
                    case SHAPE_TORUS:
                        // Uses sphere model as placeholder — torus needs GenMeshTorus in main.c
                        if (sphere_model_loaded) {
                            sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            DrawModelEx(*sphere_model, w->pos, (Vector3){0,1,0}, w->rotation_y,
                                       (Vector3){w->size.x, w->size.y * 0.3f, w->size.z}, WHITE);
                        }
                        break;
                    default: // SHAPE_CUBE
                        if (cube_model_loaded) {
                            cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            DrawModelEx(*cube_model, w->pos, (Vector3){0,1,0}, w->rotation_y,
                                       w->size, WHITE);
                        }
                        break;
                }
            } else {
                Vector3 cam = player->camera.position;
                float dx = w->pos.x - cam.x;
                float dz = w->pos.z - cam.z;
                float dist = sqrtf(dx*dx + dz*dz);
                float fog = fminf(1.0f, dist * scene->fog_density);
                Color c = w->color;
                c.r = (unsigned char)(c.r * (1 - fog) + scene->fog_color.r * fog);
                c.g = (unsigned char)(c.g * (1 - fog) + scene->fog_color.g * fog);
                c.b = (unsigned char)(c.b * (1 - fog) + scene->fog_color.b * fog);
                switch (w->shape) {
                    case SHAPE_CYLINDER:
                        DrawCylinder(w->pos, w->size.x/2, w->size.x/2, w->size.y, 16, c);
                        break;
                    case SHAPE_SPHERE:
                        DrawSphere(w->pos, w->size.x/2, c);
                        break;
                    case SHAPE_CONE:
                        DrawCylinder(w->pos, 0, w->size.x/2, w->size.y, 12, c);
                        break;
                    default:
                        DrawCube(w->pos, w->size.x, w->size.y, w->size.z, c);
                        break;
                }
            }
        }
        if (pass == 1) {
            rlDrawRenderBatchActive();  // flush before GL state change
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    // Blob shadows — dark discs under furniture, grounds objects in scene
    if (indoor && cyl_model_loaded && cyl_model && cyl_model->meshCount > 0) {
        if (lighting->ready) SetMaterialId(lighting, 0);
        int shadow_count = 0;
        for (int i = 0; i < scene->wall_count && shadow_count < 40; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active || w->shape != SHAPE_CUBE) continue;
            float bot = w->pos.y - w->size.y / 2;
            if (bot > 0.08f && bot < 2.0f && w->size.x < 4.0f && w->size.z < 4.0f
                && w->size.y > 0.1f) {
                float sx = fminf(w->size.x, 2.0f) * 0.7f;
                float sz = fminf(w->size.z, 2.0f) * 0.7f;
                float fade = 1.0f - fminf(1.0f, bot * 0.4f);
                if (fade > 0.1f) {
                    unsigned char sa = (unsigned char)(35 * fade);
                    cyl_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){0,0,0,sa};
                    DrawModelEx(*cyl_model,
                        (Vector3){w->pos.x, 0.015f, w->pos.z},
                        (Vector3){0,1,0}, 0,
                        (Vector3){sx, 0.01f, sz}, WHITE);
                    shadow_count++;
                }
            }
        }
    }

    // ── Ambient micro-animations — the world breathes ──
    // Objects that are small + above floor + not floor/ceiling/wall
    // get subtle sine-driven movement. The space is alive.
    if (!indoor) {
        // Zero-G drift — floating objects oscillate slowly
        // Modifies wall positions each frame (reverts next frame since scene rebuilds)
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            // Floating objects: small, above 1m, not a structural wall
            bool is_small = w->size.x < 0.8f && w->size.y < 0.8f && w->size.z < 0.8f;
            bool is_floating = w->pos.y > 1.0f && w->pos.y < 6.0f;
            bool is_structure = w->size.x > 3.0f || w->size.y > 3.0f || w->size.z > 3.0f;
            if (is_small && is_floating && !is_structure) {
                // Each object drifts on its own phase (seeded by index)
                float phase = (float)i * 2.3f;
                w->pos.y += sinf(time * 0.4f + phase) * 0.003f;
                w->pos.x += sinf(time * 0.25f + phase * 1.3f) * 0.002f;
                w->pos.z += cosf(time * 0.3f + phase * 0.7f) * 0.002f;
            }
        }
    }

    // Dust motes in indoor scenes
    if (indoor) {
        draw_dust_motes(player->camera, time);
    }

    // Interactive objects — diegetic, not game-y
    // "No hand-holding" — the architecture guides, not glowing beacons.
    // Only reward bloom remains: every interaction has VISIBLE PHYSICAL CONSEQUENCE.
    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active) continue;

        if (obj->done && obj->reward_timer > 0) {
            float t = 1.0f - obj->reward_timer;
            float radius = 0.3f + t * 1.5f;
            unsigned char a = (unsigned char)(150 * obj->reward_timer);
            DrawSphere(obj->pos, radius, (Color){255, 220, 120, a});
        }
        // Proximity warmth — the world acknowledges your attention
        // Not a game-y highlight — a faint warm breath, like an object
        // warming under your gaze. Diegetic: the lamp anticipates being lit.
        if (!obj->done) {
            float dist = Vector3Distance(player->camera.position, obj->pos);
            if (dist < obj->radius * 0.8f) {
                Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player->camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
                float facing = Vector3DotProduct(to_obj, look);
                if (facing > 0.6f) {
                    // Subtle warm glow — scales with proximity and facing
                    float intensity = (facing - 0.6f) / 0.4f;  // 0-1
                    float prox = 1.0f - (dist / (obj->radius * 0.8f));
                    float glow = intensity * prox * 0.3f;
                    unsigned char ga = (unsigned char)(glow * 80);
                    if (ga > 5) {
                        float pulse = 0.8f + 0.2f * sinf(time * 2.0f);
                        DrawSphere(obj->pos, 0.15f + pulse * 0.05f,
                                   (Color){255, 230, 180, (unsigned char)(ga * pulse)});
                    }
                }
            }
        }
    }

    EndMode3D();
}

// Susan Kare pixel icons — 5x5 or 7x7 symbols for each object type
static void draw_pixel_icon(int cx, int cy, const char *name, unsigned char a) {
    Color c = {248, 245, 238, a};

    if (strcmp(name, "lamp") == 0) {
        // Lightbulb: 3px top, 1px stem, 2px base
        DrawPixel(cx-1, cy-2, c); DrawPixel(cx, cy-2, c); DrawPixel(cx+1, cy-2, c);
        DrawPixel(cx-1, cy-1, c); DrawPixel(cx+1, cy-1, c);
        DrawPixel(cx, cy, c);
        DrawPixel(cx-1, cy+1, c); DrawPixel(cx+1, cy+1, c);
    } else if (strcmp(name, "candles") == 0) {
        // Flame: pointed top, wide middle, narrow base
        DrawPixel(cx, cy-2, c);
        DrawPixel(cx-1, cy-1, c); DrawPixel(cx, cy-1, c); DrawPixel(cx+1, cy-1, c);
        DrawPixel(cx, cy, c);
        DrawPixel(cx, cy+1, c);
    } else if (strcmp(name, "bed") == 0) {
        // Bed: horizontal with pillow bump
        DrawPixel(cx-2, cy-1, c); DrawPixel(cx-1, cy-1, c);
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy, c);
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy+1, c);
    } else if (strcmp(name, "drawers") == 0) {
        // 3 stacked horizontal lines
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy-2, c);
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy, c);
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy+2, c);
    } else if (strcmp(name, "ashtray") == 0) {
        // Small circle ring
        DrawPixel(cx-1, cy-1, c); DrawPixel(cx, cy-1, c); DrawPixel(cx+1, cy-1, c);
        DrawPixel(cx-1, cy, c); DrawPixel(cx+1, cy, c);
        DrawPixel(cx-1, cy+1, c); DrawPixel(cx, cy+1, c); DrawPixel(cx+1, cy+1, c);
    } else if (strcmp(name, "door") == 0 || strcmp(name, "bathroom") == 0) {
        // Rectangle with dot handle
        for (int y = -2; y <= 2; y++) { DrawPixel(cx-1, cy+y, c); DrawPixel(cx+1, cy+y, c); }
        DrawPixel(cx, cy-2, c); DrawPixel(cx, cy+2, c);
        DrawPixel(cx+1, cy, c);  // handle dot
    } else if (strcmp(name, "tap") == 0) {
        // Droplet
        DrawPixel(cx, cy-2, c);
        DrawPixel(cx-1, cy-1, c); DrawPixel(cx, cy-1, c); DrawPixel(cx+1, cy-1, c);
        DrawPixel(cx-1, cy, c); DrawPixel(cx+1, cy, c);
        DrawPixel(cx, cy+1, c);
    } else if (strcmp(name, "mirror") == 0) {
        // Vertical rectangle
        for (int y = -2; y <= 2; y++) { DrawPixel(cx-1, cy+y, c); DrawPixel(cx+1, cy+y, c); }
        DrawPixel(cx, cy-2, c); DrawPixel(cx, cy+2, c);
    } else if (strcmp(name, "wardrobe") == 0) {
        // Tall rectangle with vertical split
        for (int y = -3; y <= 2; y++) { DrawPixel(cx-2, cy+y, c); DrawPixel(cx+2, cy+y, c); }
        DrawPixel(cx-1, cy-3, c); DrawPixel(cx, cy-3, c); DrawPixel(cx+1, cy-3, c);
        DrawPixel(cx-1, cy+2, c); DrawPixel(cx, cy+2, c); DrawPixel(cx+1, cy+2, c);
        for (int y = -2; y <= 1; y++) DrawPixel(cx, cy+y, c);  // center split
    } else if (strcmp(name, "newspaper") == 0) {
        // Text lines
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy-1, c);
        for (int x = -2; x <= 1; x++) DrawPixel(cx+x, cy, c);
        for (int x = -2; x <= 2; x++) DrawPixel(cx+x, cy+1, c);
    } else if (strcmp(name, "elevator") == 0) {
        // Up/down arrows
        DrawPixel(cx, cy-2, c);
        DrawPixel(cx-1, cy-1, c); DrawPixel(cx+1, cy-1, c);
        DrawPixel(cx-1, cy+1, c); DrawPixel(cx+1, cy+1, c);
        DrawPixel(cx, cy+2, c);
    } else if (strcmp(name, "cigarette") == 0) {
        // Diagonal line with smoke dot
        DrawPixel(cx-2, cy+1, c); DrawPixel(cx-1, cy, c);
        DrawPixel(cx, cy, c); DrawPixel(cx+1, cy-1, c);
        DrawPixel(cx+2, cy-2, c);  // smoke
    } else {
        // Default: simple dot
        DrawPixel(cx, cy, c);
        DrawPixel(cx-1, cy, c); DrawPixel(cx+1, cy, c);
        DrawPixel(cx, cy-1, c); DrawPixel(cx, cy+1, c);
    }
}

// Crosshair spring state (scales up when targeting interactable)
static float crosshair_scale = 1.0f;
static float crosshair_scale_vel = 0.0f;

void draw_hud(Player *player, Scene *scene) {
    float dt = GetFrameTime();
    float target_scale = 1.0f;

    // Check if any object is targeted
    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active || obj->done) continue;
        float dist = Vector3Distance(player->camera.position, obj->pos);
        if (dist < obj->radius) {
            Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player->camera.position));
            Vector3 look = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
            float dot = Vector3DotProduct(to_obj, look);
            if (dot > 0.75f) {
                float alpha = (dot - 0.75f) / 0.25f;
                unsigned char icon_a = (unsigned char)(220 * alpha);
                target_scale = 3.0f;

                // Pixel icon below crosshair — replaces text label
                draw_pixel_icon(RENDER_W/2, RENDER_H/2 + 10, obj->name, icon_a);
            }
        }
    }

    // Spring the crosshair scale
    float ck = 280.0f, cd = 26.0f, cm = 0.9f;
    float cf = -ck * (crosshair_scale - target_scale) - cd * crosshair_scale_vel;
    crosshair_scale_vel += (cf / cm) * dt;
    crosshair_scale += crosshair_scale_vel * dt;
    if (crosshair_scale < 0.5f) crosshair_scale = 0.5f;

    // Draw spring-scaled crosshair dot
    int cs = (int)(crosshair_scale + 0.5f);
    if (cs < 1) cs = 1;
    unsigned char ca = (cs > 1) ? 120 : 60;
    DrawCircle(RENDER_W/2, RENDER_H/2, cs, (Color){220, 215, 205, ca});
}

// Title screen spring state — "PRESS ENTER" slides up from below
static float title_enter_y_offset = 20.0f;
static float title_enter_y_vel = 0.0f;
static float title_enter_scale = 0.0f;
static float title_enter_scale_vel = 0.0f;
static bool title_enter_triggered = false;

void reset_title_state(void) {
    title_enter_y_offset = 20.0f;
    title_enter_y_vel = 0.0f;
    title_enter_scale = 0.0f;
    title_enter_scale_vel = 0.0f;
    title_enter_triggered = false;
    // Also reset crosshair spring
    crosshair_scale = 1.0f;
    crosshair_scale_vel = 0.0f;
}

void draw_title(void) {
    float t = (float)GetTime();
    float dt = GetFrameTime();

    // ================================================================
    // Hotel Chevalier opening — quiet, warm, contemplative
    // Black fades to deep navy. A horizon line breathes.
    // The title materializes like phosphorescent stars.
    // Wonder and melancholy, not aggression.
    // ================================================================

    // Background: true black → deep navy over 3 seconds
    float bg_t = fminf(1.0f, t / 3.0f);
    unsigned char bg_r = (unsigned char)(5 * bg_t);
    unsigned char bg_g = (unsigned char)(5 * bg_t);
    unsigned char bg_b = (unsigned char)(8 * bg_t);
    ClearBackground((Color){bg_r, bg_g, bg_b, 255});

    // Sparse stars fade in — same seed as the ending (continuity)
    if (t > 1.0f) {
        float star_a = fminf(1.0f, (t - 1.0f) / 3.0f);
        SetRandomSeed(99);
        for (int i = 0; i < 30; i++) {
            int sx = GetRandomValue(20, RENDER_W - 20);
            int sy = GetRandomValue(10, RENDER_H * 2 / 3);
            float twk = 0.5f + 0.5f * sinf(t * (0.3f + i * 0.05f) + i * 1.7f);
            unsigned char sa = (unsigned char)(40 * star_a * twk);
            DrawPixel(sx, sy, (Color){200, 195, 180, sa});
        }
    }

    // Horizon line — golden ratio, breathes like a heartbeat
    if (t > 1.5f) {
        float line_a = fminf(1.0f, (t - 1.5f) / 2.0f);
        float breath = 0.7f + 0.3f * sinf(t * 1.2f);
        int line_y = (int)(RENDER_H * 0.5f);
        DrawRectangle(0, line_y, RENDER_W, 1,
                      (Color){178, 155, 107, (unsigned char)(120 * line_a * breath)});
    }

    // Title — "ENDEARING VOID" — materializes letter by letter
    if (t > 2.0f) {
        float title_t = t - 2.0f;
        const char *title = "ENDEARING VOID";
        int font_size = 16;
        float spacing = 14.0f;
        int len = 0;
        for (const char *p = title; *p; p++) len++;
        float total_w = (float)(len - 1) * spacing + 10.0f;
        float start_x = (float)RENDER_W / 2.0f - total_w / 2.0f;
        int title_y = (int)(RENDER_H * 0.382f);  // golden ratio

        for (int i = 0; i < len; i++) {
            // Each letter appears 0.15s after the previous
            float char_delay = (float)i * 0.15f;
            float char_t = title_t - char_delay;
            if (char_t < 0) continue;

            // Fade in over 0.8s — like phosphorescent stars charging
            float char_a = fminf(1.0f, char_t / 0.8f);
            // Subtle breathing once fully visible
            float char_breath = 1.0f;
            if (char_t > 1.0f) {
                char_breath = 0.85f + 0.15f * sinf(t * 0.8f + (float)i * 0.6f);
            }

            int cx = (int)(start_x + (float)i * spacing);
            char ch[2] = { title[i], '\0' };

            unsigned char ta = (unsigned char)(240 * char_a * char_breath);
            // Shadow
            DrawText(ch, cx + 1, title_y + 1, font_size, (Color){0, 0, 0, (unsigned char)(ta / 2)});
            // Warm gold — matches the star color progression
            DrawText(ch, cx, title_y, font_size, (Color){200, 178, 130, ta});
        }
    }

    // Credit — fades in at golden ratio below
    if (t > 5.0f) {
        float ca = fminf(1.0f, (t - 5.0f) / 2.0f);
        int credit_y = (int)(RENDER_H * 0.618f);
        draw_text_box("Maxwell Young", credit_y, 10,
                      (Color){140, 135, 128, (unsigned char)(130 * ca)});
    }

    // "PRESS ENTER" — springs in from below after 6 seconds
    if (t > 6.0f && !title_enter_triggered) {
        title_enter_triggered = true;
        title_enter_y_offset = 20.0f;
        title_enter_y_vel = 0.0f;
        title_enter_scale = 0.0f;
        title_enter_scale_vel = 0.0f;
    }

    if (title_enter_triggered) {
        // Spring physics (Kowalski: k=280, d=26, m=0.9)
        float sk = 280.0f, sd = 26.0f, sm = 0.9f;
        float fy = -sk * title_enter_y_offset - sd * title_enter_y_vel;
        title_enter_y_vel += (fy / sm) * dt;
        title_enter_y_offset += title_enter_y_vel * dt;
        float fs = -sk * (title_enter_scale - 1.0f) - sd * title_enter_scale_vel;
        title_enter_scale_vel += (fs / sm) * dt;
        title_enter_scale += title_enter_scale_vel * dt;

        if (title_enter_scale > 0.01f) {
            int enter_y = RENDER_H - 28 + (int)title_enter_y_offset;
            unsigned char ea = (unsigned char)(200 * title_enter_scale);
            if (ea > 200) ea = 200;
            // Gentle pulse — not strobe
            float pulse = 0.75f + 0.25f * sinf(t * 2.0f);
            ea = (unsigned char)((float)ea * pulse);
            draw_text_box("PRESS ENTER", enter_y, 10,
                          (Color){200, 195, 185, ea});
        }
    }
}

void draw_night_sky(float time) {
    // Gradient from deep navy (top) to dark blue-gray (horizon)
    for (int y = 0; y < RENDER_H; y++) {
        float t = (float)y / RENDER_H;
        unsigned char r = (unsigned char)(8 + t * 17);
        unsigned char g = (unsigned char)(12 + t * 18);
        unsigned char b = (unsigned char)(28 + t * 22);
        DrawRectangle(0, y, RENDER_W, 1, (Color){r, g, b, 255});
    }

    // Stars — 80 dots, seeded random, subtle twinkle
    SetRandomSeed(73);
    for (int i = 0; i < 80; i++) {
        int sx = GetRandomValue(0, RENDER_W);
        int sy = GetRandomValue(0, RENDER_H * 3 / 4);
        float bri = 0.3f + (GetRandomValue(0, 70) / 100.0f);
        float twinkle = 0.7f + 0.3f * sinf(time * (0.8f + GetRandomValue(0, 15) / 10.0f) + i * 2.1f);
        DrawPixel(sx, sy, (Color){230, 225, 215, (unsigned char)(255 * bri * twinkle)});
    }

    // Moon — upper right, pale with darker crescent overlay
    int mx = RENDER_W - 50;
    int my = 35;
    DrawCircle(mx, my, 8, (Color){220, 218, 212, 180});
    DrawCircle(mx + 3, my - 2, 7, (Color){15, 18, 35, 200});  // crescent shadow

    // City light pollution — faint warm glow along bottom quarter
    DrawRectangle(0, RENDER_H * 3 / 4, RENDER_W, RENDER_H / 4,
                  (Color){60, 45, 30, 15});
}

void draw_dawn_sky(float time) {
    (void)time;
    // Gradient: deep blue (top) → pink → gold (horizon)
    for (int y = 0; y < RENDER_H; y++) {
        float t = (float)y / RENDER_H;
        unsigned char r, g, b;
        if (t < 0.4f) {
            // Upper sky — deep blue to purple
            float u = t / 0.4f;
            r = (unsigned char)(18 + u * 30);
            g = (unsigned char)(22 + u * 20);
            b = (unsigned char)(55 + u * 15);
        } else if (t < 0.7f) {
            // Mid sky — purple to pink
            float u = (t - 0.4f) / 0.3f;
            r = (unsigned char)(48 + u * 120);
            g = (unsigned char)(42 + u * 50);
            b = (unsigned char)(70 - u * 20);
        } else {
            // Horizon — pink to warm gold
            float u = (t - 0.7f) / 0.3f;
            r = (unsigned char)(168 + u * 62);
            g = (unsigned char)(92 + u * 68);
            b = (unsigned char)(50 + u * 30);
        }
        DrawRectangle(0, y, RENDER_W, 1, (Color){r, g, b, 255});
    }
    // Faint cloud band near horizon
    DrawRectangle(0, (int)(RENDER_H * 0.72f), RENDER_W, 6,
                  (Color){220, 170, 120, 30});
    DrawRectangle(0, (int)(RENDER_H * 0.75f), RENDER_W, 4,
                  (Color){200, 150, 100, 20});
}

void draw_dust_motes(Camera3D camera, float time) {
    // ~20 small bright dots drifting near the camera in lit areas
    // Deterministic positions seeded from index, slow upward drift
    for (int i = 0; i < 20; i++) {
        // Deterministic base position relative to camera, spread out in a volume
        float seed_x = sinf((float)i * 3.7f + 0.5f) * 5.0f;
        float seed_y = fmodf((float)i * 0.37f + 0.2f, 2.5f) + 0.2f;
        float seed_z = cosf((float)i * 2.3f + 1.1f) * 5.0f;

        // Slow drift
        float drift_y = sinf(time * 0.4f + (float)i * 1.1f) * 0.3f;
        float drift_x = sinf(time * 0.25f + (float)i * 0.7f) * 0.15f;
        float drift_z = cosf(time * 0.3f + (float)i * 0.9f) * 0.15f;

        // Anchor near camera but with some spatial stability
        float anchor_x = floorf(camera.position.x / 4.0f) * 4.0f;
        float anchor_z = floorf(camera.position.z / 4.0f) * 4.0f;

        float px = anchor_x + seed_x + drift_x;
        float py = seed_y + drift_y;
        float pz = anchor_z + seed_z + drift_z;

        // Only visible in lit areas: y < 3 and within 8m of camera
        if (py > 3.0f) continue;
        float dx = px - camera.position.x;
        float dz = pz - camera.position.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 8.0f) continue;

        // Slight size variation
        float radius = 0.015f + fmodf((float)i * 0.13f, 0.015f);

        // Fade with distance
        float alpha_f = 1.0f - (dist / 8.0f);
        unsigned char a = (unsigned char)(60.0f * alpha_f);

        DrawSphere((Vector3){px, py, pz}, radius, (Color){240, 235, 225, a});
    }
}

// ── Zero-G sparkles — larger, brighter than dust motes, near portholes ──
void draw_zero_g_sparkles(Camera3D camera, float time) {
    for (int i = 0; i < 15; i++) {
        float seed_x = sinf((float)i * 5.1f + 2.3f) * 6.0f;
        float seed_y = fmodf((float)i * 0.53f + 0.4f, 3.0f) + 0.5f;
        float seed_z = cosf((float)i * 3.7f + 0.8f) * 6.0f;

        // Slower, wider drift than dust motes
        float drift_y = sinf(time * 0.2f + (float)i * 1.3f) * 0.5f;
        float drift_x = sinf(time * 0.15f + (float)i * 0.9f) * 0.3f;
        float drift_z = cosf(time * 0.18f + (float)i * 1.1f) * 0.3f;

        float anchor_x = floorf(camera.position.x / 5.0f) * 5.0f;
        float anchor_z = floorf(camera.position.z / 5.0f) * 5.0f;

        float px = anchor_x + seed_x + drift_x;
        float py = seed_y + drift_y;
        float pz = anchor_z + seed_z + drift_z;

        float dx = px - camera.position.x;
        float dz = pz - camera.position.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 10.0f) continue;

        // Occasional bright flash — shimmer
        float flash = sinf(time * 2.5f + (float)i * 4.7f);
        float brightness = (flash > 0.85f) ? 1.0f : 0.3f;

        float radius = 0.02f + fmodf((float)i * 0.17f, 0.02f);
        float alpha_f = (1.0f - dist / 10.0f) * brightness;
        unsigned char a = (unsigned char)(80.0f * alpha_f);

        DrawSphere((Vector3){px, py, pz}, radius, (Color){220, 230, 255, a});
    }
}

// ============================================================
// SHADOW PASS — render scene from light's perspective into depth buffer
//
// Uses the lightSpaceMatrix computed by UpdateShadowMatrix() — the SAME
// matrix the lighting shader uses in ShadowCalc(). This guarantees the
// depth we write here matches the coordinates the fragment shader samples.
//
// The shadow shader's mvp = lightSpaceMatrix * modelMatrix.
// No rlgl matrix stack manipulation — we set mvp directly per-draw.
// This avoids the state corruption that caused the original disable.
// ============================================================
void draw_shadow_pass(Scene *scene, EVLighting *lighting,
                      Model *cube_model, Model *cyl_model,
                      Model *sphere_model, Model *cone_model) {
    if (!lighting->shadowReady) return;

    // Flush any pending rlgl work before we touch FBO state
    rlDrawRenderBatchActive();

    // Save original shaders
    Shader origCube = cube_model->materials[0].shader;
    Shader origCyl  = cyl_model->materials[0].shader;
    Shader origSph  = sphere_model->materials[0].shader;
    Shader origCone = cone_model->materials[0].shader;

    // Assign depth-only shadow shader to all models
    cube_model->materials[0].shader   = lighting->shadowShader;
    cyl_model->materials[0].shader    = lighting->shadowShader;
    sphere_model->materials[0].shader = lighting->shadowShader;
    cone_model->materials[0].shader   = lighting->shadowShader;

    // Bind shadow FBO, set viewport, clear depth only
    rlEnableFramebuffer(lighting->shadowFBO);
    rlViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Draw all scene geometry — mvp is lightSpaceMatrix * modelMatrix
    Matrix lsm = lighting->lightSpaceMatrix;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;

        Matrix matScale = MatrixScale(w->size.x, w->size.y, w->size.z);
        Matrix matRot = MatrixRotateY(w->rotation_y * DEG2RAD);
        Matrix matTrans = MatrixTranslate(w->pos.x, w->pos.y, w->pos.z);
        Matrix matModel = MatrixMultiply(MatrixMultiply(matScale, matRot), matTrans);

        // shadow mvp = lightSpaceMatrix * modelMatrix
        Matrix shadowMvp = MatrixMultiply(matModel, lsm);
        SetShaderValueMatrix(lighting->shadowShader, lighting->shadowMvpLoc, shadowMvp);

        Model *m;
        switch (w->shape) {
            case SHAPE_CYLINDER: m = cyl_model; break;
            case SHAPE_SPHERE:   m = sphere_model; break;
            case SHAPE_CONE:     m = cone_model; break;
            default:             m = cube_model; break;
        }

        // Identity model matrix — full transform is baked into the shader's mvp
        DrawMesh(m->meshes[0], m->materials[0], MatrixIdentity());
    }

    // Flush ALL draw calls before restoring state
    rlDrawRenderBatchActive();

    // Unbind shadow FBO and restore viewport
    rlDisableFramebuffer();
    rlViewport(0, 0, RENDER_W, RENDER_H);

    // Restore original shaders
    cube_model->materials[0].shader   = origCube;
    cyl_model->materials[0].shader    = origCyl;
    sphere_model->materials[0].shader = origSph;
    cone_model->materials[0].shader   = origCone;

    lighting->shadowPassRan = true;
}

// ============================================================
// PROCEDURAL EARTH — the emotional anchor of the game
// Multi-layered: deep ocean, continent hints, cloud wisps,
// atmosphere rim glow, city lights on the night side.
// At 480x300, the sphere is ~100px across — every layer counts.
// ============================================================
void draw_earth(Camera3D camera, float time,
                Model *sphere_model, EVLighting *lighting,
                Vector3 earth_center) {
    if (!sphere_model) return;

    Vector3 earth_pos = earth_center;
    float earth_scale = 50.0f;
    float rot = time * 0.5f;  // slow rotation — ~12 min per revolution
    Vector3 axis = {0.23f, 1, 0};  // tilted axis (Earth's 23.5°)
    Color saved_color = sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color;

    // Layer 1: Deep ocean — dark navy-blue base
    if (lighting && lighting->ready) SetMaterialId(lighting, 0);  // concrete — matte ocean
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){18, 42, 110, 255};
    DrawModelEx(*sphere_model, earth_pos, axis, rot,
                (Vector3){earth_scale, earth_scale, earth_scale}, WHITE);

    // Layer 2: Continent landmasses — slightly larger sphere, green-brown tint
    // The overlap with the ocean sphere creates a layered, painterly look
    if (lighting && lighting->ready) SetMaterialId(lighting, 3);  // carpet — matte land texture
    float land_scale = earth_scale * 1.002f;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){55, 80, 45, 160};
    DrawModelEx(*sphere_model, earth_pos, axis, rot + 15.0f,  // offset rotation = different "continents"
                (Vector3){land_scale, land_scale * 0.97f, land_scale}, WHITE);

    // Layer 3: Ice caps — poles slightly brighter
    float ice_scale = earth_scale * 1.003f;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){200, 210, 220, 80};
    DrawModelEx(*sphere_model, earth_pos, axis, rot,
                (Vector3){ice_scale * 0.6f, ice_scale * 1.15f, ice_scale * 0.6f}, WHITE);

    // Layer 4: Cloud layer — slightly larger, slowly drifting
    if (lighting && lighting->ready) SetMaterialId(lighting, 4);  // wallpaper — swirling pattern
    float cloud_scale = earth_scale * 1.012f;
    float cloud_rot = rot + time * 0.3f;  // clouds drift faster than surface
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){220, 225, 235, 35};
    DrawModelEx(*sphere_model, earth_pos, axis, cloud_rot,
                (Vector3){cloud_scale, cloud_scale, cloud_scale}, WHITE);

    // Layer 5: City lights on the night side — warm amber dots
    // Slightly smaller sphere, bright warm color, only visible where unlit
    if (lighting && lighting->ready) SetMaterialId(lighting, 0);  // concrete for flat rendering
    float city_scale = earth_scale * 1.004f;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 200, 80, 25};
    DrawModelEx(*sphere_model, earth_pos, axis, rot + 8.0f,
                (Vector3){city_scale, city_scale * 0.95f, city_scale}, WHITE);

    // Layer 6: Atmosphere rim — Fresnel-like glow, blue-white edge
    // Thin shell that catches the light shader's rim calculation
    if (lighting && lighting->ready) SetMaterialId(lighting, 7);  // glass — reflective rim
    float atmo_scale = earth_scale * 1.06f;
    // Breathing: atmosphere subtly pulses — the planet is alive
    float breath = 1.0f + sinf(time * 0.4f) * 0.008f;
    atmo_scale *= breath;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){100, 160, 240, 30};
    DrawModelEx(*sphere_model, earth_pos, axis, rot * 0.1f,
                (Vector3){atmo_scale, atmo_scale, atmo_scale}, WHITE);

    // Layer 7: Outer glow — largest, faintest, catches edge light
    float glow_scale = earth_scale * 1.12f;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){80, 140, 220, 12};
    DrawModelEx(*sphere_model, earth_pos, axis, 0,
                (Vector3){glow_scale, glow_scale, glow_scale}, WHITE);

    // Earth-shine on the observation deck — 2D glow on lower screen
    // Subtle blue wash from the planet's reflected light
    {
        float glow_y = (float)RENDER_H * 0.7f;
        float glow_h = (float)RENDER_H * 0.3f;
        float pulse = 0.7f + 0.3f * sinf(time * 0.6f);
        unsigned char ga = (unsigned char)(12.0f * pulse);
        EndMode3D();  // switch to 2D for the glow overlay
        DrawRectangle(0, (int)glow_y, RENDER_W, (int)glow_h,
                      (Color){40, 100, 200, ga});
        BeginMode3D(camera);  // return to 3D
    }

    // Restore
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = saved_color;
}

void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target) {
    if (!pfx->ready) return;
    float t = (float)GetTime();
    SetShaderValue(pfx->postfx, pfx->timeLoc, &t, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(pfx->postfx);
    DrawTexturePro(render_target.texture,
        (Rectangle){0, 0, RENDER_W, -RENDER_H},
        (Rectangle){0, 0, RENDER_W, RENDER_H},
        (Vector2){0, 0}, 0, WHITE);
    EndShaderMode();
}
