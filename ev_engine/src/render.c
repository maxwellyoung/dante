// render.c — Rendering pipeline with post-processing
#include "render.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

// ============================================================
// POST-PROCESSING SHADER
// Vignette + color grading + film grain + Bayer dither
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
    "out vec4 finalColor;\n"
    "\n"
    "// Bayer 4x4 matrix for ordered dithering\n"
    "float bayer4(vec2 p) {\n"
    "    int x = int(mod(p.x, 4.0));\n"
    "    int y = int(mod(p.y, 4.0));\n"
    "    int index = x + y * 4;\n"
    "    float m[16] = float[16](\n"
    "         0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,\n"
    "        12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,\n"
    "         3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,\n"
    "        15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0\n"
    "    );\n"
    "    return m[index];\n"
    "}\n"
    "\n"
    "// Film grain\n"
    "float hash(vec2 p) {\n"
    "    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec4 tex = texture(texture0, fragTexCoord);\n"
    "    vec3 col = tex.rgb;\n"
    "\n"
    "    // --- Color grading: warm tint + lifted blacks ---\n"
    "    col = pow(col, vec3(0.95, 1.0, 1.05));  // slight warm shift\n"
    "    col = col * 0.92 + vec3(0.025, 0.02, 0.015);  // lift blacks warm\n"
    "\n"
    "    // --- Vignette ---\n"
    "    vec2 uv = fragTexCoord;\n"
    "    float vig = 1.0 - dot((uv - 0.5) * 1.2, (uv - 0.5) * 1.2);\n"
    "    vig = clamp(pow(vig, 0.8), 0.0, 1.0);\n"
    "    col *= mix(0.55, 1.0, vig);  // darken edges\n"
    "\n"
    "    // --- Film grain ---\n"
    "    float grain = hash(fragTexCoord * resolution + vec2(time * 100.0, time * 73.0));\n"
    "    grain = (grain - 0.5) * 0.06;  // subtle\n"
    "    col += grain;\n"
    "\n"
    "    // --- Bayer dither (quantize to ~5 bit) ---\n"
    "    vec2 pixelCoord = fragTexCoord * resolution;\n"
    "    float dith = bayer4(pixelCoord) - 0.5;\n"
    "    col += dith * (1.0 / 32.0);  // ~5-bit color banding\n"
    "\n"
    "    // --- Scanline hint (very subtle) ---\n"
    "    float scan = 1.0 - 0.04 * mod(pixelCoord.y, 2.0);\n"
    "    col *= scan;\n"
    "\n"
    "    finalColor = vec4(clamp(col, 0.0, 1.0), tex.a);\n"
    "}\n";

EVPostFX LoadEVPostFX(void) {
    EVPostFX pfx = {0};
    pfx.postfx = LoadShaderFromMemory(postfx_vs, postfx_fs);
    if (pfx.postfx.id > 0) {
        pfx.timeLoc = GetShaderLocation(pfx.postfx, "time");
        pfx.resolutionLoc = GetShaderLocation(pfx.postfx, "resolution");
        float res[2] = {(float)RENDER_W, (float)RENDER_H};
        SetShaderValue(pfx.postfx, pfx.resolutionLoc, res, SHADER_UNIFORM_VEC2);
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

void draw_scene_3d(Player *player, Scene *scene, EVLighting *lighting,
                   Model *cube_model, bool cube_model_loaded) {
    ClearBackground(scene->fog_color);

    if (lighting->ready) {
        UpdateEVLighting(lighting, player->camera, scene->fog_color, scene->fog_density);
    }

    BeginMode3D(player->camera);

    // Draw walls
    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;

        if (lighting->ready && cube_model_loaded) {
            cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = w->color;
            DrawModelEx(*cube_model, w->pos, (Vector3){0,1,0}, 0,
                       w->size, WHITE);
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

    // Interactive objects — pulsing glow
    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active || obj->done) continue;
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 2.5f + i * 1.7f);
        Color c = obj->color;
        c.a = (unsigned char)(180 * pulse);
        DrawSphere(obj->pos, 0.2f, c);
        DrawSphere(obj->pos, 0.4f, (Color){c.r, c.g, c.b, (unsigned char)(25 * pulse)});
    }

    // Exit beacon
    if (scene->has_exit) {
        float pulse = 0.4f + 0.3f * sinf(GetTime() * 2);
        DrawSphere(scene->exit_pos, 0.3f, (Color){230, 200, 100, (unsigned char)(80 * pulse)});
        DrawSphere(scene->exit_pos, 0.6f, (Color){230, 200, 100, (unsigned char)(20 * pulse)});
    }

    EndMode3D();
}

void draw_dither_overlay(void) {
    // Kept for fallback — post-processing shader handles dithering now
}

void draw_hud(Player *player, Scene *scene, GameState state, int tasks_done, int total_tasks,
              const char *screen_text, float screen_text_timer) {
    // Interaction prompt
    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active || obj->done) continue;
        float dist = Vector3Distance(player->camera.position, obj->pos);
        if (dist < obj->radius) {
            Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player->camera.position));
            Vector3 look = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
            float dot = Vector3DotProduct(to_obj, look);
            if (dot > 0.7f) {
                DrawRectangle(RENDER_W/2 - 90, RENDER_H/2 + 20, 180, 24, (Color){0, 0, 0, 140});
                DrawText(TextFormat("[E] %s", obj->prompt),
                         RENDER_W/2 - 80, RENDER_H/2 + 25, 10, (Color){240, 230, 210, 230});
            }
        }
    }

    // Task counter
    if (state == STATE_ROOM) {
        DrawText(TextFormat("%d/%d", tasks_done, total_tasks),
                 RENDER_W - 45, 10, 10, (Color){190, 155, 90, 150});
    }

    // Screen text
    if (screen_text && screen_text_timer > 0) {
        float alpha = screen_text_timer < 0.5f ? screen_text_timer / 0.5f : 1.0f;
        DrawRectangle(0, RENDER_H - 36, RENDER_W, 28, (Color){0, 0, 0, (unsigned char)(100 * alpha)});
        DrawText(screen_text, 14, RENDER_H - 30, 10, (Color){230, 225, 215, (unsigned char)(230 * alpha)});
    }
}

void draw_title(float planet_angle) {
    ClearBackground((Color){8, 10, 22, 255});

    SetRandomSeed(42);
    for (int i = 0; i < 120; i++) {
        int sx = GetRandomValue(0, RENDER_W);
        int sy = GetRandomValue(0, RENDER_H);
        float bri = 0.3f + (GetRandomValue(0, 100) / 100.0f) * 0.7f;
        float twinkle = 0.7f + 0.3f * sinf(GetTime() * (1 + GetRandomValue(0, 20) / 10.0f) + i);
        DrawPixel(sx, sy, (Color){240, 235, 220, (unsigned char)(255 * bri * twinkle)});
    }

    float cx = RENDER_W / 2, cy = RENDER_H / 2 + 15;
    DrawCircle((int)cx, (int)cy, 45, (Color){60, 55, 72, 255});
    for (int i = 0; i < 6; i++) {
        float crat_x = sinf(i * 1.7f + planet_angle) * 25;
        float crat_y = cosf(i * 2.3f) * 18;
        if (sqrtf(crat_x*crat_x + crat_y*crat_y) < 38) {
            DrawCircle((int)(cx + crat_x), (int)(cy + crat_y), 3 + i % 3, (Color){45, 40, 55, 150});
        }
    }
    DrawCircleLines((int)cx, (int)cy, 47, (Color){100, 110, 150, 40});

    const char *title = "E N D E A R I N G   V O I D";
    int tw = MeasureText(title, 12);
    DrawText(title, RENDER_W/2 - tw/2, 25, 12, (Color){240, 232, 210, 230});

    const char *sub = "A game by Glitched Games";
    int sw = MeasureText(sub, 8);
    DrawText(sub, RENDER_W/2 - sw/2, RENDER_H - 35, 8, (Color){180, 175, 165, 140});

    float pulse = 0.4f + 0.4f * sinf(GetTime() * 3);
    const char *prompt = "PRESS ENTER";
    int pw = MeasureText(prompt, 10);
    DrawText(prompt, RENDER_W/2 - pw/2, RENDER_H - 20, 10,
             (Color){230, 190, 90, (unsigned char)(255 * pulse)});
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
