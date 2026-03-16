// render.c — Rendering pipeline with post-processing
// No hand-holding. No task counter. No exit beacon.
// The space speaks for itself.
#include "render.h"
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
                // Object name below crosshair
                int tw = MeasureText(obj->name, 8);
                DrawText(obj->name, RENDER_W/2 - tw/2, RENDER_H/2 + 8, 8,
                    (Color){245, 238, 225, (unsigned char)(200 * alpha)});
            }
        }
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
