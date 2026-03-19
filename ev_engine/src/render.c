// render.c — Rendering pipeline with post-processing
// No hand-holding. No task counter. No exit beacon.
// The space speaks for itself.
#include "render.h"
#include "game_ctx.h"
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
    "    // --- FXAA — edge-aware anti-aliasing ---\n"
    "    vec3 fxaa;\n"
    "    {\n"
    "        float FXAA_SPAN = 4.0;\n"
    "        float FXAA_MIN = 1.0/64.0;\n"
    "        float FXAA_MUL = 1.0/8.0;\n"
    "        vec3 cNW = texture(texture0, uv + vec2(-1,-1)*px).rgb;\n"
    "        vec3 cNE = texture(texture0, uv + vec2( 1,-1)*px).rgb;\n"
    "        vec3 cSW = texture(texture0, uv + vec2(-1, 1)*px).rgb;\n"
    "        vec3 cSE = texture(texture0, uv + vec2( 1, 1)*px).rgb;\n"
    "        vec3 cM  = texture(texture0, uv).rgb;\n"
    "        vec3 lumaW = vec3(0.299, 0.587, 0.114);\n"
    "        float lNW = dot(cNW, lumaW);\n"
    "        float lNE = dot(cNE, lumaW);\n"
    "        float lSW = dot(cSW, lumaW);\n"
    "        float lSE = dot(cSE, lumaW);\n"
    "        float lM  = dot(cM,  lumaW);\n"
    "        float lMin = min(lM, min(min(lNW,lNE), min(lSW,lSE)));\n"
    "        float lMax = max(lM, max(max(lNW,lNE), max(lSW,lSE)));\n"
    "        vec2 dir = vec2(-(lNW+lNE)+(lSW+lSE), (lNW+lSW)-(lNE+lSE));\n"
    "        float dirReduce = max((lNW+lNE+lSW+lSE)*0.25*FXAA_MUL, FXAA_MIN);\n"
    "        float rcpDir = 1.0/(min(abs(dir.x),abs(dir.y))+dirReduce);\n"
    "        dir = clamp(dir*rcpDir, vec2(-FXAA_SPAN), vec2(FXAA_SPAN)) * px;\n"
    "        vec3 a = 0.5*(texture(texture0,uv+dir*(1.0/3.0-0.5)).rgb+\n"
    "                      texture(texture0,uv+dir*(2.0/3.0-0.5)).rgb);\n"
    "        vec3 b2 = a*0.5+0.25*(texture(texture0,uv+dir*-0.5).rgb+\n"
    "                              texture(texture0,uv+dir*0.5).rgb);\n"
    "        float lB = dot(b2, lumaW);\n"
    "        fxaa = (lB<lMin||lB>lMax) ? a : b2;\n"
    "    }\n"
    "\n"
    "    // --- Chromatic aberration — RGB split at edges ---\n"
    "    float caStrength = dot(fromCenter, fromCenter) * caAmount;\n"
    "    vec2 caOffset = fromCenter * caStrength * px * 3.0;\n"
    "    float r = texture(texture0, uv + caOffset).r;\n"
    "    float g = texture(texture0, uv).g;\n"
    "    float b = texture(texture0, uv - caOffset).b;\n"
    "    vec3 col = vec3(r, g, b);\n"
    "    // Blend FXAA — smooth edges while preserving CA character\n"
    "    col = mix(col, fxaa, 0.3);\n"
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
    "    // --- Bloom — HDR-aware, wider kernel, threshold-based ---\n"
    "    // Two-pass Gaussian approximation in a single pass: sample at wider offsets\n"
    "    // with Gaussian weights. Threshold isolates bright sources (lights, specular).\n"
    "    if (bloomAmt > 0.0) {\n"
    "        vec3 bloomCol = vec3(0.0);\n"
    "        float totalW = 0.0;\n"
    "        // Sample in a cross pattern at increasing distances (fake separable blur)\n"
    "        for (int i = -8; i <= 8; i++) {\n"
    "            float w = exp(-float(i*i) / 12.0);\n"  // Gaussian weight, sigma≈3.5 (wider glow)
    "            vec3 sH = texture(texture0, uv + vec2(float(i), 0) * px * 3.0).rgb;\n"
    "            vec3 sV = texture(texture0, uv + vec2(0, float(i)) * px * 3.0).rgb;\n"
    "            // Threshold — only bloom bright pixels (light sources, specular)\n"
    "            float lH = dot(sH, vec3(0.299, 0.587, 0.114));\n"
    "            float lV = dot(sV, vec3(0.299, 0.587, 0.114));\n"
    "            bloomCol += sH * max(lH - 0.35, 0.0) * w;\n"
    "            bloomCol += sV * max(lV - 0.35, 0.0) * w;\n"
    "            totalW += w * 2.0;\n"
    "        }\n"
    "        bloomCol /= totalW;\n"
    "        col += bloomCol * bloomAmt * 4.5;\n"
    "    }\n"
    "\n"
    "    // --- SSAO — subtle edge darkening (light touch at 960x600) ---\n"
    "    float center_luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    float ao_accum = 0.0;\n"
    "    for (int i = 0; i < 8; i++) {\n"
    "        float angle = float(i) * 0.785398;\n"
    "        vec2 offset = vec2(cos(angle), sin(angle)) * px * 1.5;\n"
    "        float neighbor_luma = dot(texture(texture0, uv + offset).rgb, vec3(0.299, 0.587, 0.114));\n"
    "        ao_accum += max(0.0, center_luma - neighbor_luma);\n"
    "    }\n"
    "    float ssao = 1.0 - ao_accum * 0.28;\n"
    "    col *= max(ssao, 0.65);\n"
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
    "    // --- Split toning — gentle, only engages with warmth progression ---\n"
    "    {\n"
    "        float st_luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "        vec3 shadow_tint = vec3(0.95, 0.97, 1.03);\n"
    "        vec3 highlight_tint = vec3(1.02, 1.0, 0.97);\n"
    "        float shadow_w = 1.0 - smoothstep(0.0, 0.5, st_luma);\n"
    "        float highlight_w = smoothstep(0.5, 1.0, st_luma);\n"
    "        // Only kicks in as warmth rises — cold scenes stay neutral\n"
    "        float split_strength = warmth * 0.35;\n"
    "        col *= mix(vec3(1.0), shadow_tint, shadow_w * split_strength);\n"
    "        col *= mix(vec3(1.0), highlight_tint, highlight_w * split_strength);\n"
    "    }\n"
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
    "    // --- Vignette — gentle, not crushing ---\n"
    "    float vig = 1.0 - dot((uv - 0.5) * 1.0, (uv - 0.5) * 1.0);\n"
    "    vig = clamp(pow(vig, 0.7), 0.0, 1.0);\n"
    "    float vigDark = mix(0.85, 1.0, vig);\n"
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
    "    // --- ACES Filmic Tonemapping ---\n"
    "    // Replaces hard clamp — preserves highlights, compresses brights naturally.\n"
    "    // This is the single biggest visual upgrade vs naive clamping.\n"
    "    {\n"
    "        // ACES approximation (Krzysztof Narkowicz fit)\n"
    "        float a = 2.51;\n"
    "        float b = 0.03;\n"
    "        float c = 2.43;\n"
    "        float d = 0.59;\n"
    "        float e = 0.14;\n"
    "        col = (col * (a * col + b)) / (col * (c * col + d) + e);\n"
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
        float defaultGrain = 0.3f;
        SetShaderValue(pfx.postfx, pfx.grainLoc, &defaultGrain, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.flashLoc, &zero, SHADER_UNIFORM_FLOAT);
        float white[3] = {1.0f, 1.0f, 1.0f};
        SetShaderValue(pfx.postfx, pfx.flashColorLoc, white, SHADER_UNIFORM_VEC3);
        float defaultSat = 1.0f;
        SetShaderValue(pfx.postfx, pfx.saturationLoc, &defaultSat, SHADER_UNIFORM_FLOAT);
        float defaultCA = 0.8f;
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
    // 1: Default — clean architectural photograph. Let the geometry speak.
    //    Minimal processing. Colors stay true. Grain is texture, not noise.
    {"16mm Film",     1.0f,  0.9f, 1.0f, 0.75f, 0.32f,  0.10f, {1.02f,1.0f,0.98f},    0.0f,0.0f,0.30f, 0,  1,  0.35f},

    // 2: PS1 — ordered dithering, color quantization, chunky pixels
    {"PS1",           0.85f, 0.5f, 0.8f, 0.4f, 0.1f,  0.05f,{1.0f,0.98f,0.95f},      1.0f,0.0f,0.0f, 12, 2,  0.0f},

    // 3: Noir — crushed blacks, nearly mono, heavy vignette, sharp
    {"Noir",          0.15f, 0.8f, 1.6f, 1.8f, 0.6f, -0.15f, {0.94f,0.94f,1.0f},     0.0f,0.0f,0.0f, 0,  1,  1.2f},

    // 4: CRT — scanlines, bloom, phosphor glow
    {"CRT",           0.95f, 2.0f, 1.0f, 1.0f, 0.2f,  0.05f,{1.0f,0.95f,0.9f},       0.0f,0.8f,0.4f, 0,  1,  0.0f},

    // 5: Godard — saturated, contrasty, red push, French New Wave
    {"Godard",        1.25f, 1.5f, 1.3f, 0.8f, 0.5f,  0.1f, {1.12f,0.96f,0.88f},     0.0f,0.0f,0.2f, 0,  1,  0.4f},

    // 6: VHS — grain, CA, warm, scanlines
    {"VHS",           0.75f, 5.0f, 0.6f, 1.2f, 1.0f, -0.1f, {1.08f,0.96f,0.88f},     0.15f,0.5f,0.15f, 0,  1,  0.0f},

    // 7: Neon — oversaturated, bloom, teal-orange
    {"Neon",          1.35f, 1.5f, 1.1f, 0.5f, 0.15f, 0.15f,{1.08f,0.92f,1.12f},     0.0f,0.0f,0.8f, 0,  1,  0.2f},

    // 8: Woodcut — extreme dithering, near-mono, posterized
    {"Woodcut",       0.1f,  0.3f, 1.8f, 1.2f, 0.0f,  0.0f, {1.0f,0.98f,0.95f},      1.5f,0.0f,0.0f, 4,  1,  1.8f},

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
    font_size *= UI_SCALE;
    if (font_size < 12 * UI_SCALE) font_size = 12 * UI_SCALE;
    int tw = MeasureText(text, font_size);
    int tx = RENDER_W / 2 - tw / 2;
    int pad = 12 * UI_SCALE;
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
        UpdateEVLighting(lighting, player->camera, scene->fog_color, scene->fog_density, time);
        AnimateLights(lighting, time);
    }

    BeginMode3D(player->camera);

    // Draw walls — two passes: solids first, then decals with polygon offset
    // This prevents z-fighting on overlay geometry (rugs, floor details, trim)
    // Distance culling: skip walls > 80m from camera (saves draw calls in dense scenes)
    float cam_x = player->camera.position.x;
    float cam_y = player->camera.position.y;
    float cam_z = player->camera.position.z;
    float cull_dist_sq = 80.0f * 80.0f;

    bool has_decals = false;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            if (!has_decals) break;
            rlDrawRenderBatchActive();
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-3.0f, -3.0f);  // strong bias — ACES tonemapping exposes subtle z-fighting
        }
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            if (pass == 0 && w->is_decal) { has_decals = true; continue; }
            if (pass == 1 && !w->is_decal) continue;

            // Distance culling — skip walls far from camera
            float dx = w->pos.x - cam_x;
            float dy = w->pos.y - cam_y;
            float dz = w->pos.z - cam_z;
            float dist_sq = dx*dx + dy*dy + dz*dz;
            // Don't cull very large walls (size > 10m — floors, ceilings, skyboxes)
            float max_dim = w->size.x > w->size.y ? (w->size.x > w->size.z ? w->size.x : w->size.z) : (w->size.y > w->size.z ? w->size.y : w->size.z);
            if (dist_sq > cull_dist_sq && max_dim < 10.0f) continue;

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
                    case SHAPE_MODEL: {
                        extern GameCtx g;
                        int mi = w->model_index;
                        if (mi >= 0 && mi < g.model_asset_count && g.model_assets[mi].loaded) {
                            ModelAsset *ma = &g.model_assets[mi];
                            // Multi-material models (GLB): draw with their own Blender colors.
                            // Single-color override only if the wall color isn't white (255,255,255).
                            bool override_color = !(w->color.r == 255 && w->color.g == 255 && w->color.b == 255);
                            if (override_color) {
                                // Simple prop — override all material slots with wall color
                                for (int mj = 0; mj < ma->model.materialCount; mj++)
                                    ma->model.materials[mj].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                            }
                            DrawModelEx(ma->model, w->pos, (Vector3){0,1,0}, w->rotation_y,
                                       w->size, WHITE);
                        }
                        break;
                    }
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
    // Polygon offset prevents z-fighting with floor geometry
    if (indoor && cyl_model_loaded && cyl_model && cyl_model->meshCount > 0) {
        rlDrawRenderBatchActive();
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-2.0f, -2.0f);  // stronger bias than decals — shadows always on top
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
        rlDrawRenderBatchActive();
        glDisable(GL_POLYGON_OFFSET_FILL);
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
    // Rain in outdoor scenes (exterior only — not space)
    if (!indoor && player->camera.position.y < 50.0f) {
        draw_rain(player->camera, time);
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
    // At higher res, each "pixel" becomes a UI_SCALE×UI_SCALE block
    #define DP(px, py) DrawRectangle((px), (py), UI_SCALE, UI_SCALE, c)
    #undef DrawPixel
    #define DrawPixel(px, py, col) DP(px, py)

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
    #undef DrawPixel
    #undef DP
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
                draw_pixel_icon(RENDER_W/2, RENDER_H/2 + 10 * UI_SCALE, obj->name, icon_a);

                // "E" prompt — small, below icon
                const char *prompt = "E";
                int pw = MeasureText(prompt, 10 * UI_SCALE);
                DrawText(prompt, RENDER_W/2 - pw/2, RENDER_H/2 + 22 * UI_SCALE, 10 * UI_SCALE,
                         (Color){220, 215, 205, icon_a});
            }
        }
    }

    // Check if aiming at a grabbable (pushable) wall
    if (target_scale < 1.5f) {
        Vector3 origin = player->camera.position;
        Vector3 dir = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->pushable || !w->active) continue;
            Vector3 to_w = Vector3Subtract(w->pos, origin);
            float along = Vector3DotProduct(to_w, dir);
            if (along < 0 || along > 2.5f) continue;
            Vector3 closest = Vector3Add(origin, Vector3Scale(dir, along));
            float perp = Vector3Distance(closest, w->pos);
            float radius = fmaxf(w->size.x, fmaxf(w->size.y, w->size.z)) * 0.6f;
            if (perp < radius) { target_scale = 2.0f; break; }
        }
    }

    // Spring the crosshair scale
    float ck = 280.0f, cd = 26.0f, cm = 0.9f;
    float cf = -ck * (crosshair_scale - target_scale) - cd * crosshair_scale_vel;
    crosshair_scale_vel += (cf / cm) * dt;
    crosshair_scale += crosshair_scale_vel * dt;
    if (crosshair_scale < 0.5f) crosshair_scale = 0.5f;

    // Draw spring-scaled crosshair dot
    int cs = (int)(crosshair_scale * UI_SCALE + 0.5f);
    if (cs < UI_SCALE) cs = UI_SCALE;
    unsigned char ca = (cs > UI_SCALE) ? 120 : 60;
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
    // Un Chien Andalou × Godard × American Zoetrope
    //
    // Phase 1: Studio card — "ninetynine digital" — warm gold on black,
    //          prestige framing lines, like Zoetrope before Apocalypse Now
    // Phase 2: The Moon — full white disc, wispy clouds drift across,
    //          faithful to Buñuel's actual sequence
    // Phase 3: The Razor — diagonal slash across the moon,
    //          then the moon splits and slides apart
    // Phase 4: RED flash — Godard's red, not white. Le Mépris.
    // Phase 5: Title card — "ENDEARING VOID" in clean white,
    //          red rule underneath (tricolore energy)
    // ================================================================

    ClearBackground(BLACK);

    int cx = RENDER_W / 2;
    int cy = RENDER_H / 2;

    // Godard palette
    Color godard_red  = {195, 45, 40, 255};
    Color godard_blue = {40, 65, 160, 255};

    // ── Phase 1: Studio card (0 → 3.2s) ────────────────────────────
    // American Zoetrope energy: prestige, gold, framing lines
    if (t < 3.2f) {
        float fade_in = fminf(1.0f, t / 1.0f);
        float fade_out = (t > 2.4f) ? 1.0f - fminf(1.0f, (t - 2.4f) / 0.8f) : 1.0f;
        float a = fade_in * fade_out;
        unsigned char ua = (unsigned char)(255 * a);

        // Framing lines — top and bottom, golden, cinematic aspect ratio
        int frame_inset = 80 * UI_SCALE;
        int line_top = cy - 45 * UI_SCALE;
        int line_bot = cy + 45 * UI_SCALE;
        DrawRectangle(frame_inset, line_top, RENDER_W - frame_inset * 2, 1,
                      (Color){178, 155, 107, (unsigned char)(140 * a)});
        DrawRectangle(frame_inset, line_bot, RENDER_W - frame_inset * 2, 1,
                      (Color){178, 155, 107, (unsigned char)(140 * a)});

        // Studio name — warm gold, generous tracking
        const char *studio = "NINETYNINE DIGITAL";
        int sfs = 12 * UI_SCALE;
        int sw = MeasureText(studio, sfs);
        DrawText(studio, cx - sw / 2, cy - 6 * UI_SCALE, sfs,
                 (Color){210, 185, 120, ua});

        // Small decorative dot above and below name (Zoetrope motif)
        DrawCircle(cx, line_top + 6*UI_SCALE, 2*UI_SCALE, (Color){178, 155, 107, (unsigned char)(100 * a)});
        DrawCircle(cx, line_bot - 6*UI_SCALE, 2*UI_SCALE, (Color){178, 155, 107, (unsigned char)(100 * a)});

        return;
    }

    // ── Phase 2: The Moon (3.2 → 5.6s) ─────────────────────────────
    // Buñuel's actual image: full moon, clouds drifting across
    if (t < 5.6f) {
        float mt = t - 3.2f;  // 0 → 2.4

        // Moon — large white disc, slightly off-center (cinematic framing)
        int moon_x = cx;
        int moon_y = cy - 10 * UI_SCALE;
        int moon_r = 55 * UI_SCALE;

        // Moon fades in
        float moon_a = fminf(1.0f, mt / 0.8f);
        unsigned char ma = (unsigned char)(240 * moon_a);

        // Moon glow — soft halo
        for (int ring = moon_r + 20; ring > moon_r; ring--) {
            float glow_f = 1.0f - (float)(ring - moon_r) / 20.0f;
            unsigned char ga = (unsigned char)(30 * glow_f * moon_a);
            DrawCircleLines(moon_x, moon_y, ring, (Color){200, 210, 230, ga});
        }

        // Moon disc — slightly blue-white (moonlight)
        DrawCircle(moon_x, moon_y, moon_r, (Color){220, 225, 235, ma});

        // Moon surface — subtle crater shadows (seeded)
        SetRandomSeed(42);
        for (int i = 0; i < 8; i++) {
            int dx = GetRandomValue(-moon_r + 15, moon_r - 15);
            int dy = GetRandomValue(-moon_r + 15, moon_r - 15);
            if (dx * dx + dy * dy < (moon_r - 12) * (moon_r - 12)) {
                int cr = GetRandomValue(4, 12);
                DrawCircle(moon_x + dx, moon_y + dy, cr,
                           (Color){195, 200, 210, (unsigned char)(40 * moon_a)});
            }
        }

        // Clouds — horizontal wisps drifting right-to-left across the moon
        // Three cloud bands at different speeds (parallax)
        for (int c = 0; c < 3; c++) {
            float speed = 35.0f + (float)c * 15.0f;
            float cloud_x = (float)RENDER_W - mt * speed + (float)c * 200.0f;
            int cloud_y = moon_y - 15 + c * 18;
            int cloud_w = 180 + c * 40;
            int cloud_h = 8 + c * 3;

            // Cloud is a soft rect with alpha falloff
            for (int row = 0; row < cloud_h; row++) {
                float row_f = (float)row / (float)cloud_h;
                // Bell curve across height
                float density = 1.0f - 4.0f * (row_f - 0.5f) * (row_f - 0.5f);
                if (density < 0) density = 0;
                unsigned char ca = (unsigned char)(120 * density * moon_a);
                // Horizontal soft edges
                for (int col = 0; col < cloud_w; col++) {
                    float col_f = (float)col / (float)cloud_w;
                    float edge = 1.0f - 4.0f * (col_f - 0.5f) * (col_f - 0.5f);
                    if (edge < 0) edge = 0;
                    int px = (int)cloud_x + col;
                    int py = cloud_y + row;
                    if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H) {
                        unsigned char pa = (unsigned char)((float)ca * edge);
                        if (pa > 4)
                            DrawPixel(px, py, (Color){15, 18, 25, pa});
                    }
                }
            }
        }

        // The razor — diagonal slash at mt=1.8, Buñuel's actual diagonal
        if (mt > 1.8f) {
            float razor_t = fminf(1.0f, (mt - 1.8f) / 0.25f);
            // Diagonal line from upper-left to lower-right across moon
            int x1 = moon_x - moon_r - 20;
            int y1 = moon_y - moon_r - 20;
            int x2 = moon_x + moon_r + 20;
            int y2 = moon_y + moon_r + 20;
            // Interpolate endpoint
            int ex = x1 + (int)((float)(x2 - x1) * razor_t);
            int ey = y1 + (int)((float)(y2 - y1) * razor_t);
            // Bright white razor line
            DrawLine(x1, y1, ex, ey, (Color){255, 255, 255, 255});
            DrawLine(x1 + 1, y1, ex + 1, ey, (Color){255, 255, 255, 180});
            // Red trail — Godard's blood
            DrawLine(x1 - 1, y1 + 1, ex - 1, ey + 1,
                     (Color){godard_red.r, godard_red.g, godard_red.b, (unsigned char)(200 * razor_t)});
        }

        // After razor completes — moon splits apart (the wound)
        if (mt > 2.1f) {
            float split_t = fminf(1.0f, (mt - 2.1f) / 0.3f);
            float eased = split_t * split_t;
            int split_dist = (int)(25.0f * eased);
            // Black wedge opens along the diagonal cut
            for (int i = -moon_r - 10; i < moon_r + 10; i++) {
                int bx = moon_x + i;
                int by = moon_y + i;  // diagonal
                for (int w = 0; w < split_dist; w++) {
                    int px = bx - w / 2;
                    int py = by + w / 2;
                    if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H)
                        DrawPixel(px, py, BLACK);
                    px = bx + w / 2;
                    py = by - w / 2;
                    if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H)
                        DrawPixel(px, py, BLACK);
                }
            }
        }

        return;
    }

    // ── Phase 3: Red flash — Godard (5.6 → 5.9s) ───────────────────
    if (t < 5.9f) {
        float flash_t = (t - 5.6f) / 0.3f;
        if (flash_t < 0.4f) {
            // RED flash — Le Mépris, Pierrot le Fou
            float fi = flash_t / 0.4f;
            ClearBackground((Color){
                (unsigned char)(godard_red.r * fi),
                (unsigned char)(godard_red.g * fi),
                (unsigned char)(godard_red.b * fi), 255});
        } else {
            // Decay through blue to black — tricolore
            float fd = (flash_t - 0.4f) / 0.6f;
            float blue_peak = (fd < 0.3f) ? fd / 0.3f : 1.0f - (fd - 0.3f) / 0.7f;
            if (blue_peak < 0) blue_peak = 0;
            float red_decay = 1.0f - fd;
            if (red_decay < 0) red_decay = 0;
            ClearBackground((Color){
                (unsigned char)(godard_red.r * red_decay * 0.3f),
                (unsigned char)(godard_blue.g * blue_peak * 0.6f),
                (unsigned char)(godard_blue.b * blue_peak * 0.4f), 255});
        }
        return;
    }

    // ── Phase 4: Title card (5.9s+) ─────────────────────────────────
    {
        float title_a = fminf(1.0f, (t - 5.9f) / 0.5f);
        unsigned char ta = (unsigned char)(255 * title_a);

        const char *title = "ENDEARING VOID";
        int font_size = 30 * UI_SCALE;
        int tw = MeasureText(title, font_size);
        int tx = cx - tw / 2;
        int ty = (int)(RENDER_H * 0.40f);

        // Title — bright white, bold, confident
        DrawText(title, tx, ty, font_size, (Color){250, 248, 242, ta});

        // Red rule underneath — Godard accent
        if (title_a > 0.2f) {
            float rule_a = fminf(1.0f, (title_a - 0.2f) / 0.3f);
            int rule_w = tw;
            int rule_x = cx - rule_w / 2;
            int rule_y = ty + font_size + 6 * UI_SCALE;
            DrawRectangle(rule_x, rule_y, rule_w, 2 * UI_SCALE,
                          (Color){godard_red.r, godard_red.g, godard_red.b,
                           (unsigned char)(220 * rule_a)});
        }

        // Blue accent dot — subtle, lower right of rule (tricolore hint)
        if (title_a > 0.5f) {
            float dot_a = fminf(1.0f, (title_a - 0.5f) / 0.3f);
            int rule_y = ty + font_size + 6;
            DrawCircle(cx + tw / 2 + 8, rule_y + 1, 2,
                       (Color){godard_blue.r, godard_blue.g, godard_blue.b,
                        (unsigned char)(160 * dot_a)});
        }
    }

    // ── "PRESS ENTER" — springs in after title settles ──────────────
    if (t > 7.5f && !title_enter_triggered) {
        title_enter_triggered = true;
        title_enter_y_offset = 15.0f;
        title_enter_y_vel = 0.0f;
        title_enter_scale = 0.0f;
        title_enter_scale_vel = 0.0f;
    }

    if (title_enter_triggered) {
        float sk = 280.0f, sd = 26.0f, sm = 0.9f;
        float fy = -sk * title_enter_y_offset - sd * title_enter_y_vel;
        title_enter_y_vel += (fy / sm) * dt;
        title_enter_y_offset += title_enter_y_vel * dt;
        float fs = -sk * (title_enter_scale - 1.0f) - sd * title_enter_scale_vel;
        title_enter_scale_vel += (fs / sm) * dt;
        title_enter_scale += title_enter_scale_vel * dt;

        if (title_enter_scale > 0.01f) {
            float sc = fminf(title_enter_scale, 1.0f);
            unsigned char ea = (unsigned char)(220 * sc);
            float pulse = 0.88f + 0.12f * sinf(t * 1.8f);
            ea = (unsigned char)((float)ea * pulse);
            int ey = RENDER_H - 50*UI_SCALE + (int)(title_enter_y_offset * UI_SCALE);
            const char *prompt = "PRESS ENTER";
            int pfs = 10 * UI_SCALE;
            int pw = MeasureText(prompt, pfs);
            DrawText(prompt, cx - pw / 2, ey, pfs,
                     (Color){200, 198, 190, ea});
        }
    }
}

// draw_night_sky and draw_dawn_sky removed — replaced by GPU skybox shader (skybox.c)

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
        if (dist > 12.0f) continue;

        // Slight size variation
        float radius = 0.018f + fmodf((float)i * 0.13f, 0.018f);

        // Fade with distance
        float alpha_f = 1.0f - (dist / 12.0f);
        unsigned char a = (unsigned char)(70.0f * alpha_f);

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

// ── Rain — thin falling lines, Auckland 2AM ──
void draw_rain(Camera3D camera, float time) {
    // Rain around the camera — short white lines falling at slight angle
    for (int i = 0; i < MAX_RAIN; i++) {
        // Deterministic seed per drop, wrapping in a volume around camera
        float seed_x = sinf((float)i * 7.3f + 0.5f) * 15.0f;
        float seed_z = cosf((float)i * 4.1f + 1.7f) * 15.0f;

        // Anchor to camera grid so rain moves with you
        float anchor_x = floorf(camera.position.x / 8.0f) * 8.0f;
        float anchor_z = floorf(camera.position.z / 8.0f) * 8.0f;

        float px = anchor_x + seed_x;
        float pz = anchor_z + seed_z;

        // Cycling fall: each drop falls from 8m to ground, loops
        float phase = fmodf((float)i * 0.17f, 1.0f);
        float fall_speed = 6.0f + fmodf((float)i * 0.31f, 2.0f);
        float py = 8.0f - fmodf(time * fall_speed + phase * 8.0f, 8.0f);

        // Distance cull
        float dx = px - camera.position.x;
        float dz = pz - camera.position.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 18.0f) continue;

        // Rain streak — short line, slight wind angle
        float len = 0.3f + fmodf((float)i * 0.23f, 0.2f);
        float wind_x = 0.04f;  // slight eastward lean
        float alpha_f = 1.0f - (dist / 18.0f);
        unsigned char a = (unsigned char)(35.0f * alpha_f);

        DrawLine3D(
            (Vector3){px, py, pz},
            (Vector3){px + wind_x, py - len, pz},
            (Color){180, 190, 210, a}
        );
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
// PROCEDURAL EARTH — the emotional anchor
// Simplified: 3 layers that don't fight each other.
// Ocean base, atmosphere rim, outer glow. That's it.
// At 960x600 the sphere is ~100-200px. Clean reads better.
// ============================================================
void draw_earth(Camera3D camera, float time,
                Model *sphere_model, EVLighting *lighting,
                Vector3 earth_center) {
    if (!sphere_model) return;

    Vector3 earth_pos = earth_center;
    float earth_scale = 50.0f;
    float rot = time * 0.5f;
    Vector3 axis = {0.23f, 1, 0};  // 23.5° axial tilt
    Color saved_color = sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color;

    // Layer 1: Planet body — MAT_EMISSIVE bypasses room lighting entirely.
    // Earth is in space, not in the room. What you set is what you get.
    if (lighting && lighting->ready) SetMaterialId(lighting, 16);  // MAT_EMISSIVE
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){22, 52, 120, 255};
    DrawModelEx(*sphere_model, earth_pos, axis, rot,
                (Vector3){earth_scale, earth_scale, earth_scale}, WHITE);

    // Layer 2: Atmosphere — emissive, immune to room lighting
    if (lighting && lighting->ready) SetMaterialId(lighting, 16);  // MAT_EMISSIVE
    float atmo_scale = earth_scale * 1.04f;
    float breath = 1.0f + sinf(time * 0.4f) * 0.006f;
    atmo_scale *= breath;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){90, 155, 235, 45};
    DrawModelEx(*sphere_model, earth_pos, axis, rot * 0.1f,
                (Vector3){atmo_scale, atmo_scale, atmo_scale}, WHITE);

    // Layer 3: Outer glow — soft halo, defines the planet against void
    float glow_scale = earth_scale * 1.10f;
    sphere_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){70, 130, 210, 15};
    DrawModelEx(*sphere_model, earth_pos, axis, 0,
                (Vector3){glow_scale, glow_scale, glow_scale}, WHITE);

    // Earthshine — 2D blue wash on lower screen
    {
        float glow_y = (float)RENDER_H * 0.7f;
        float glow_h = (float)RENDER_H * 0.3f;
        float pulse = 0.7f + 0.3f * sinf(time * 0.6f);
        unsigned char ga = (unsigned char)(20.0f * pulse);
        EndMode3D();
        DrawRectangle(0, (int)glow_y, RENDER_W, (int)glow_h,
                      (Color){50, 110, 220, ga});
        BeginMode3D(camera);
    }

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
