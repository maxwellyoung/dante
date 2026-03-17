// render.c — Rendering pipeline with post-processing
// No hand-holding. No task counter. No exit beacon.
// The space speaks for itself.
#include "render.h"
#include "palette.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

// ============================================================
// POST-PROCESSING SHADER
// Clean architectural photograph aesthetic
// warmth uniform shifts color temperature as room tasks complete
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
    "out vec4 finalColor;\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 px = 1.0 / resolution;\n"
    "\n"
    "    // --- Fake depth-of-field — blur toward edges for low-luminance pixels ---\n"
    "    vec3 col = texture(texture0, uv).rgb;\n"
    "    float edgeDist = length(uv - 0.5);\n"
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
    "    // --- Saturation — slightly desaturated, architectural ---\n"
    "    float luma = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    col = mix(vec3(luma), col, 0.92 + warmth * 0.08);\n"
    "\n"
    "    // --- Color grading — warmth shift ---\n"
    "    col = pow(col, vec3(0.97 + warmth*0.03, 1.0, 1.02 - warmth*0.05));\n"
    "    col = col * 0.97 + vec3(0.02, 0.018, 0.015);\n"
    "\n"
    "    // --- Gentle contrast curve — photograph S-curve ---\n"
    "    col = smoothstep(0.0, 1.0, col * 1.05 - 0.02);\n"
    "\n"
    "    // --- Vignette — barely perceptible framing ---\n"
    "    float vig = 1.0 - dot((uv - 0.5) * 0.9, (uv - 0.5) * 0.9);\n"
    "    vig = clamp(pow(vig, 0.8), 0.0, 1.0);\n"
    "    col *= mix(0.88, 1.0, vig);\n"
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
        float res[2] = {(float)RENDER_W, (float)RENDER_H};
        SetShaderValue(pfx.postfx, pfx.resolutionLoc, res, SHADER_UNIFORM_VEC2);
        float w = 0.0f;
        SetShaderValue(pfx.postfx, pfx.warmthLoc, &w, SHADER_UNIFORM_FLOAT);
        pfx.ready = true;
        printf("[EV] Post-processing shader loaded\n");
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

void draw_text_box(const char *text, int y, int font_size, Color text_color) {
    if (!text) return;
    if (font_size < 10) font_size = 10;
    int tw = MeasureText(text, font_size);
    int tx = RENDER_W / 2 - tw / 2;
    int pad = 8;
    // Solid dark background bar — full screen width
    DrawRectangle(0, y - pad, RENDER_W, font_size + pad * 2,
                  (Color){12, 12, 14, 230});
    // Shadow — 1px offset
    DrawText(text, tx + 1, y + 1, font_size, PAL_TEXT_SHADOW);
    // Text
    DrawText(text, tx, y, font_size, text_color);
}

void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded,
                   Model *cyl_model, bool cyl_model_loaded,
                   Model *sphere_model, bool sphere_model_loaded,
                   Model *cone_model, bool cone_model_loaded,
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

void draw_hud(Player *player, Scene *scene) {
    // Interaction hint — shows object name when looking at it
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
                // Brighter, larger crosshair dot when aimed at something
                DrawCircle(RENDER_W/2, RENDER_H/2 + 1, 3,
                    (Color){255, 248, 235, (unsigned char)(180 * alpha)});
                // Object name below crosshair — with shadow
                int tw = MeasureText(obj->name, 10);
                DrawText(obj->name, RENDER_W/2 - tw/2 + 1, RENDER_H/2 + 9, 10,
                    (Color){0, 0, 0, (unsigned char)(180 * alpha)});
                DrawText(obj->name, RENDER_W/2 - tw/2, RENDER_H/2 + 8, 10,
                    (Color){245, 238, 225, (unsigned char)(200 * alpha)});
            }
        }
    }
}

void draw_title(void) {
    ClearBackground((Color){5, 5, 8, 255});

    float t = (float)GetTime();
    // Fade-in alpha for the horizon line (ramps over first 2 seconds)
    float line_alpha = fminf(1.0f, t / 2.0f);

    // Horizon line — 1px, warm brass, centered vertically
    int line_y = RENDER_H / 2;
    DrawRectangle(0, line_y, RENDER_W, 1,
                  (Color){178, 155, 107, (unsigned char)(180 * line_alpha)});

    // Title — above the line
    if (line_alpha > 0.1f) {
        draw_text_box("E N D E A R I N G   V O I D", RENDER_H / 2 - 10, 16,
                      (Color){200, 178, 130, (unsigned char)(230 * line_alpha)});
    }

    // Studio name — below the line
    if (line_alpha > 0.1f) {
        draw_text_box("Maxwell Young", RENDER_H / 2 + 10, 10,
                      (Color){140, 135, 128, (unsigned char)(120 * line_alpha)});
    }

    // Press enter — bottom, pulsing gently
    float pulse = 0.5f + 0.3f * sinf(t * 2.5f);
    if (line_alpha > 0.1f) {
        draw_text_box("PRESS ENTER", RENDER_H - 22, 10,
                      (Color){235, 228, 218, (unsigned char)(200 * pulse * line_alpha)});
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
