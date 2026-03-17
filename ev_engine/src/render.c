// render.c — Rendering pipeline with post-processing
// No hand-holding. No task counter. No exit beacon.
// The space speaks for itself.
#include "render.h"
#include "palette.h"
#include "raymath.h"
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
    "out vec4 finalColor;\n"
    "\n"
    // Film grain — hash-based noise, no textures
    "float grainHash(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.x + p3.y) * p3.z);\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 px = 1.0 / resolution;\n"
    "    vec2 fromCenter = uv - 0.5;\n"
    "\n"
    "    // --- Chromatic aberration — subtle RGB split at edges ---\n"
    "    float caStrength = dot(fromCenter, fromCenter) * 2.5;\n"
    "    vec2 caOffset = fromCenter * caStrength * px * 3.0;\n"
    "    float r = texture(texture0, uv + caOffset).r;\n"
    "    float g = texture(texture0, uv).g;\n"
    "    float b = texture(texture0, uv - caOffset).b;\n"
    "    vec3 col = vec3(r, g, b);\n"
    "\n"
    "    // --- Fake depth-of-field — blur toward edges for low-luminance pixels ---\n"
    "    float edgeDist = length(fromCenter);\n"
    "    float lumaCenter = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    float dofRadius = edgeDist * (1.0 - lumaCenter) * 2.0;\n"
    "    dofRadius = clamp(dofRadius, 0.0, 2.0);\n"
    "    if (dofRadius > 0.1) {\n"
    "        vec3 sum = col;\n"
    "        sum += texture(texture0, uv + vec2( dofRadius,  0.0) * px).rgb;\n"
    "        sum += texture(texture0, uv + vec2(-dofRadius,  0.0) * px).rgb;\n"
    "        sum += texture(texture0, uv + vec2( 0.0,  dofRadius) * px).rgb;\n"
    "        sum += texture(texture0, uv + vec2( 0.0, -dofRadius) * px).rgb;\n"
    "        col = sum / 5.0;\n"
    "    }\n"
    "\n"
    "    // --- Screen-space edge darkening (SSAO approximation) ---\n"
    "    float center_luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    float ao_accum = 0.0;\n"
    "    for (int i = 0; i < 8; i++) {\n"
    "        float angle = float(i) * 0.785398;\n"
    "        vec2 offset = vec2(cos(angle), sin(angle)) * px * 2.5;\n"
    "        float neighbor_luma = dot(texture(texture0, uv + offset).rgb, vec3(0.299, 0.587, 0.114));\n"
    "        ao_accum += max(0.0, center_luma - neighbor_luma);\n"
    "    }\n"
    "    float ssao = 1.0 - ao_accum * 0.35;\n"
    "    col *= max(ssao, 0.5);\n"
    "\n"
    "    // --- Exposure — global brightness ---\n"
    "    col *= exp2(exposure);\n"
    "\n"
    "    // --- Saturation — slightly desaturated, architectural ---\n"
    "    float luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    col = mix(vec3(luma), col, 0.92 + warmth * 0.08);\n"
    "\n"
    "    // --- Color grading — warmth shift ---\n"
    "    col = pow(max(col, vec3(0.0)), vec3(0.97 + warmth*0.03, 1.0, 1.02 - warmth*0.05));\n"
    "    col = col * 0.97 + vec3(0.02, 0.018, 0.015);\n"
    "\n"
    "    // --- Gentle contrast curve — photograph S-curve ---\n"
    "    col = smoothstep(0.0, 1.0, col * 1.05 - 0.02);\n"
    "\n"
    "    // --- Film grain — 16mm Godard stock ---\n"
    "    if (grain > 0.0) {\n"
    "        float n = grainHash(uv * resolution + fract(time * 7.3) * 100.0);\n"
    "        n = (n - 0.5) * grain * 0.12;\n"
    "        float shadow = 1.0 - smoothstep(0.0, 0.4, luma);\n"
    "        col += n * (0.5 + shadow * 0.5);\n"
    "    }\n"
    "\n"
    "    // --- Vignette — barely perceptible framing ---\n"
    "    float vig = 1.0 - dot((uv - 0.5) * 0.9, (uv - 0.5) * 0.9);\n"
    "    vig = clamp(pow(vig, 0.8), 0.0, 1.0);\n"
    "    col *= mix(0.88, 1.0, vig);\n"
    "\n"
    "    // --- Scene-cut flash — Blendo smash cut ---\n"
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

        float res[2] = {(float)RENDER_W, (float)RENDER_H};
        SetShaderValue(pfx.postfx, pfx.resolutionLoc, res, SHADER_UNIFORM_VEC2);

        // Defaults
        float zero = 0.0f;
        SetShaderValue(pfx.postfx, pfx.warmthLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.exposureLoc, &zero, SHADER_UNIFORM_FLOAT);
        float defaultGrain = 0.5f;  // subtle 16mm grain always on
        SetShaderValue(pfx.postfx, pfx.grainLoc, &defaultGrain, SHADER_UNIFORM_FLOAT);
        SetShaderValue(pfx.postfx, pfx.flashLoc, &zero, SHADER_UNIFORM_FLOAT);
        float white[3] = {1.0f, 1.0f, 1.0f};
        SetShaderValue(pfx.postfx, pfx.flashColorLoc, white, SHADER_UNIFORM_VEC3);

        pfx.ready = true;
        printf("[EV] Post-FX loaded — grain, CA, exposure, flash\n");
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

    // Draw walls
    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;

        if (lighting->ready) {
            switch (w->shape) {
                case SHAPE_CYLINDER:
                    if (cyl_model_loaded) {
                        cyl_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                        DrawModelEx(*cyl_model, w->pos, (Vector3){0,1,0}, 0,
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
                        DrawModelEx(*cone_model, w->pos, (Vector3){0,1,0}, 0,
                                   (Vector3){w->size.x, w->size.y, w->size.z}, WHITE);
                    }
                    break;
                case SHAPE_SKYTOWER:
                    if (skytower_model_loaded) {
                        skytower_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                        // size.x = uniform scale, model is ~9.5 units tall at scale 1
                        DrawModelEx(*skytower_model, w->pos, (Vector3){0,1,0}, 0,
                                   (Vector3){w->size.x, w->size.x, w->size.x}, WHITE);
                    }
                    break;
                default: // SHAPE_CUBE
                    if (cube_model_loaded) {
                        cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
                        DrawModelEx(*cube_model, w->pos, (Vector3){0,1,0}, 0,
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
            DrawCube(w->pos, w->size.x, w->size.y, w->size.z, c);
        }
    }

    // Dust motes in indoor scenes
    if (indoor) {
        draw_dust_motes(player->camera, time);
    }

    // Interactive objects — warm inviting glow, golden bloom on reward
    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active) continue;

        if (obj->done) {
            // Reward bloom — expanding golden ring that fades
            if (obj->reward_timer > 0) {
                float t = 1.0f - obj->reward_timer;
                float radius = 0.3f + t * 1.5f;
                unsigned char a = (unsigned char)(150 * obj->reward_timer);
                DrawSphere(obj->pos, radius, (Color){255, 220, 120, a});
            }
            continue;
        }

        float pulse = 0.6f + 0.3f * sinf(GetTime() * 1.8f + i * 1.7f);
        Color c = obj->color;
        c.a = (unsigned char)(100 * pulse);
        DrawSphere(obj->pos, 0.25f, c);
        DrawSphere(obj->pos, 0.6f, (Color){c.r, c.g, c.b, (unsigned char)(12 * pulse)});
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

void draw_title(void) {
    ClearBackground((Color){5, 5, 8, 255});

    float t = (float)GetTime();
    float dt = GetFrameTime();

    // Fade-in alpha (ramps over first 2 seconds)
    float line_alpha = fminf(1.0f, t / 2.0f);

    // Golden ratio layout: title at 38.2%, line at 50%, credit at 61.8%
    int title_y = (int)(RENDER_H * 0.382f);
    int line_y = RENDER_H / 2;
    int credit_y = (int)(RENDER_H * 0.618f);

    // --- Horizon line — breathing pulse like a heartbeat ---
    float breath = 0.7f + 0.3f * sinf(t * 1.2f);
    DrawRectangle(0, line_y, RENDER_W, 1,
                  (Color){178, 155, 107, (unsigned char)(180 * line_alpha * breath)});

    // --- Title — spatial typography (Cooper): each char with breathing spacing ---
    if (line_alpha > 0.1f) {
        const char *title = "ENDEARING VOID";
        int font_size = 16;
        float base_spacing = 14.0f;
        int len = 0;
        for (const char *p = title; *p; p++) len++;
        float total_w = (float)(len - 1) * base_spacing + 10.0f;
        float start_x = (float)RENDER_W / 2.0f - total_w / 2.0f;
        unsigned char ta = (unsigned char)(240 * line_alpha);

        for (int i = 0; i < len; i++) {
            float char_offset = 0.5f * sinf(t * 0.8f + (float)i * 0.6f);
            int cx = (int)(start_x + (float)i * base_spacing + char_offset);
            char ch[2] = { title[i], '\0' };
            DrawText(ch, cx + 1, title_y + 1, font_size, (Color){0, 0, 0, 255});
            DrawText(ch, cx, title_y, font_size, (Color){200, 178, 130, ta});
        }
    }

    // --- Studio name — at golden ratio below ---
    if (line_alpha > 0.1f) {
        draw_text_box("Maxwell Young", credit_y, 12,
                      (Color){140, 135, 128, (unsigned char)(130 * line_alpha)});
    }

    // --- "PRESS ENTER" — springs in from below after 2 seconds ---
    if (t > 2.0f && !title_enter_triggered) {
        title_enter_triggered = true;
        title_enter_y_offset = 20.0f;
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
            int enter_y = RENDER_H - 28 + (int)title_enter_y_offset;
            unsigned char ea = (unsigned char)(220 * title_enter_scale);
            if (ea > 220) ea = 220;
            float pulse = 0.85f + 0.15f * sinf(t * 2.0f);
            ea = (unsigned char)((float)ea * pulse);
            draw_text_box("PRESS ENTER", enter_y, 12,
                          (Color){248, 245, 238, ea});
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
