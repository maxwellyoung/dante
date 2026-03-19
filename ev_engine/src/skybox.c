// skybox.c — Procedural skybox shader for EV engine
// Replaces CPU per-scanline gradients + per-pixel star loops with a single full-screen quad.
// Matching pattern from lighting.c: GLSL embedded as C string literals.
#include "skybox.h"
#include "ev_types.h"
#include "rlgl.h"
#include "raymath.h"
#include <stdio.h>

static const char *skybox_vs =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *skybox_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform float time;\n"
    "uniform int skyType;\n"
    "uniform vec2 resolution;\n"
    "uniform mat4 viewMatrix;\n"
    "uniform float param1;\n"
    "uniform float param2;\n"
    "out vec4 finalColor;\n"
    "\n"
    // Noise functions — same as lighting.c
    "float hash(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.x + p3.y) * p3.z);\n"
    "}\n"
    "float noise(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    f = f * f * (3.0 - 2.0 * f);\n"
    "    float a = hash(i);\n"
    "    float b = hash(i + vec2(1.0, 0.0));\n"
    "    float c = hash(i + vec2(0.0, 1.0));\n"
    "    float d = hash(i + vec2(1.0, 1.0));\n"
    "    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);\n"
    "}\n"
    "float fbm(vec2 p) {\n"
    "    float v = 0.0;\n"
    "    v += noise(p) * 0.5;\n"
    "    v += noise(p * 2.0) * 0.25;\n"
    "    v += noise(p * 4.0) * 0.125;\n"
    "    v += noise(p * 8.0) * 0.0625;\n"
    "    return v;\n"
    "}\n"
    "\n"
    // Star field — stable angular hash (no jitter as camera rotates)
    "float star_hash(vec2 cell) {\n"
    "    return fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    // Reconstruct view direction from UV + inverse view rotation\n"
    "    // This gives natural parallax as camera turns\n"
    "    float aspect = resolution.x / resolution.y;\n"
    "    float fov_rad = radians(70.0);\n"
    "    float half_h = tan(fov_rad * 0.5);\n"
    "    vec3 rd = normalize(vec3((uv.x - 0.5) * aspect * half_h * 2.0,\n"
    "                             (uv.y - 0.5) * half_h * 2.0,\n"
    "                             -1.0));\n"
    "    // Apply inverse view rotation for parallax\n"
    "    rd = (inverse(mat3(viewMatrix)) * rd);\n"
    "\n"
    "    vec3 col = vec3(0.0);\n"
    "\n"
    "    // === SKY_AUCKLAND_NIGHT (1) ===\n"
    "    if (skyType == 1) {\n"
    "        // Navy gradient — deep top, blue-gray horizon\n"
    "        float gy = 1.0 - uv.y;\n"  // flip: top=0, bottom=1
    "        vec3 top = vec3(8.0, 12.0, 28.0) / 255.0;\n"
    "        vec3 bot = vec3(25.0, 30.0, 50.0) / 255.0;\n"
    "        col = mix(top, bot, gy);\n"
    "\n"
    "        // Stars — stable via angular hash on view direction\n"
    "        // Project rd onto a sphere grid\n"
    "        float phi = atan(rd.z, rd.x);\n"
    "        float theta = asin(clamp(rd.y, -1.0, 1.0));\n"
    "        vec2 sky_uv = vec2(phi, theta) * 20.0;\n"
    "        vec2 cell = floor(sky_uv);\n"
    "        float h = star_hash(cell);\n"
    "        if (h > 0.92 && theta > 0.0) {\n"
    "            vec2 offset = vec2(star_hash(cell + 0.5), star_hash(cell + 1.3)) - 0.5;\n"
    "            float d = length(fract(sky_uv) - 0.5 - offset * 0.3);\n"
    "            float bri = smoothstep(0.06, 0.0, d);\n"
    "            float twinkle = 0.7 + 0.3 * sin(time * (0.8 + h * 2.0) + h * 50.0);\n"
    "            col += vec3(0.9, 0.88, 0.84) * bri * twinkle * (0.3 + h * 0.7);\n"
    "        }\n"
    "\n"
    "        // Moon — upper right region\n"
    "        vec2 moon_dir = normalize(vec2(0.7, 0.6));\n"
    "        float moon_ang = dot(normalize(rd.xz), moon_dir);\n"
    "        if (moon_ang > 0.99 && rd.y > 0.3) {\n"
    "            float moon_d = length(vec2(moon_ang - 1.0, rd.y - 0.45)) * 40.0;\n"
    "            float moon = smoothstep(1.0, 0.3, moon_d);\n"
    "            float crescent = smoothstep(0.6, 0.3, length(vec2(moon_ang - 0.995, rd.y - 0.47)) * 40.0);\n"
    "            col += vec3(0.86, 0.85, 0.83) * moon * (1.0 - crescent * 0.9) * 0.7;\n"
    "        }\n"
    "\n"
    "        // City light pollution glow along horizon\n"
    "        float horizon_glow = exp(-max(0.0, rd.y) * 8.0);\n"
    "        col += vec3(0.24, 0.18, 0.12) * horizon_glow * 0.06;\n"
    "\n"
    "        // Param1 fade (elevator ascent: fade out as you rise)\n"
    "        col *= (1.0 - param1);\n"
    "    }\n"
    "\n"
    "    // === SKY_SPACE_VOID (2) ===\n"
    "    else if (skyType == 2) {\n"
    "        // Blue-purple gradient with depth\n"
    "        float gy = 1.0 - uv.y;\n"
    "        vec3 top = vec3(4.0, 5.0, 12.0) / 255.0;\n"
    "        vec3 bot = vec3(7.0, 9.0, 20.0) / 255.0;\n"
    "        col = mix(top, bot, gy);\n"
    "\n"
    "        // Two-layer parallax stars — bright + dim\n"
    "        for (int layer = 0; layer < 2; layer++) {\n"
    "            float scale = (layer == 0) ? 30.0 : 50.0;\n"
    "            float threshold = (layer == 0) ? 0.93 : 0.95;\n"
    "            float phi = atan(rd.z, rd.x);\n"
    "            float theta = asin(clamp(rd.y, -1.0, 1.0));\n"
    "            vec2 sky_uv = vec2(phi, theta) * scale;\n"
    "            vec2 cell = floor(sky_uv);\n"
    "            float h = star_hash(cell + float(layer) * 100.0);\n"
    "            if (h > threshold) {\n"
    "                vec2 offset = vec2(star_hash(cell + 0.5), star_hash(cell + 1.3)) - 0.5;\n"
    "                float d = length(fract(sky_uv) - 0.5 - offset * 0.3);\n"
    "                float bri = smoothstep(0.05, 0.0, d);\n"
    "                float twinkle = 0.6 + 0.4 * sin(time * (0.3 + h * 1.5) + h * 40.0);\n"
    "                // Color variation: some warm, some cool\n"
    "                vec3 star_col = (mod(h * 100.0, 5.0) < 1.0)\n"
    "                    ? vec3(1.0, 0.85, 0.7)   // warm\n"
    "                    : (mod(h * 100.0, 7.0) < 1.0)\n"
    "                        ? vec3(0.7, 0.85, 1.0) // cool\n"
    "                        : vec3(0.9, 0.9, 0.9); // neutral\n"
    "                float intensity = (layer == 0) ? 0.8 : 0.4;\n"
    "                col += star_col * bri * twinkle * (0.3 + h * 0.7) * intensity;\n"
    "                // Bright stars get a glow\n"
    "                if (bri > 0.5 && layer == 0) {\n"
    "                    col += star_col * smoothstep(0.15, 0.0, d) * 0.08 * twinkle;\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "\n"
    "        // Nebula wisps — subtle fbm clouds\n"
    "        float phi2 = atan(rd.z, rd.x);\n"
    "        float theta2 = asin(clamp(rd.y, -1.0, 1.0));\n"
    "        float nebula = fbm(vec2(phi2 * 3.0 + time * 0.01, theta2 * 4.0));\n"
    "        col += vec3(0.08, 0.05, 0.12) * smoothstep(0.4, 0.7, nebula) * 0.3;\n"
    "    }\n"
    "\n"
    "    // === SKY_DAWN (3) ===\n"
    "    else if (skyType == 3) {\n"
    "        float gy = 1.0 - uv.y;\n"
    "        vec3 top_col = vec3(18.0, 22.0, 55.0) / 255.0;\n"
    "        vec3 mid_col = vec3(48.0, 42.0, 70.0) / 255.0;\n"
    "        vec3 pink = vec3(168.0, 92.0, 50.0) / 255.0;\n"
    "        vec3 gold = vec3(230.0, 160.0, 80.0) / 255.0;\n"
    "        if (gy < 0.4) {\n"
    "            col = mix(top_col, mid_col, gy / 0.4);\n"
    "        } else if (gy < 0.7) {\n"
    "            col = mix(mid_col, pink, (gy - 0.4) / 0.3);\n"
    "        } else {\n"
    "            col = mix(pink, gold, (gy - 0.7) / 0.3);\n"
    "        }\n"
    "        // Cloud bands near horizon — fbm\n"
    "        float cloud = fbm(vec2(uv.x * 8.0 + time * 0.03, gy * 2.0));\n"
    "        float cloud_mask = smoothstep(0.65, 0.8, gy) * smoothstep(1.0, 0.85, gy);\n"
    "        col += vec3(0.86, 0.67, 0.47) * cloud * cloud_mask * 0.12;\n"
    "        // Warm horizon rim\n"
    "        col += vec3(0.9, 0.6, 0.3) * smoothstep(0.85, 1.0, gy) * 0.15;\n"
    "    }\n"
    "\n"
    "    // === SKY_PARIS_DREAM (4) ===\n"
    "    else if (skyType == 4) {\n"
    "        // Golden volumetric haze — animated fbm fog\n"
    "        float gy = 1.0 - uv.y;\n"
    "        vec3 base = vec3(35.0, 28.0, 15.0) / 255.0;\n"
    "        vec3 haze = vec3(0.6, 0.45, 0.2);\n"
    "        float fog = fbm(vec2(uv.x * 4.0 + time * 0.05, gy * 3.0 + time * 0.02));\n"
    "        float fog2 = fbm(vec2(uv.x * 6.0 - time * 0.03, gy * 2.0 + time * 0.04));\n"
    "        col = base + haze * (fog * 0.15 + fog2 * 0.08);\n"
    "        // Warm center glow\n"
    "        float center_d = length(uv - vec2(0.5, 0.6));\n"
    "        col += vec3(0.5, 0.35, 0.15) * smoothstep(0.5, 0.0, center_d) * 0.08;\n"
    "    }\n"
    "\n"
    "    // === SKY_HYPERSPACE (5) ===\n"
    "    else if (skyType == 5) {\n"
    "        // Radial streaks from center — speed from param1\n"
    "        vec2 center = uv - 0.5;\n"
    "        float angle = atan(center.y, center.x);\n"
    "        float dist = length(center);\n"
    "        float speed = max(0.5, param1);\n"
    "        // Streak pattern — angular hash + radial motion\n"
    "        float streak_angle = angle * 30.0;\n"
    "        float streak = hash(vec2(floor(streak_angle), 0.0));\n"
    "        float radial = fract(dist * 3.0 - time * speed * 0.5 + streak * 5.0);\n"
    "        float intensity = streak * smoothstep(0.0, 0.15, radial) * smoothstep(0.4, 0.15, radial);\n"
    "        intensity *= smoothstep(0.0, 0.2, dist);  // fade center\n"
    "        col = vec3(2.0, 2.0, 6.0) / 255.0;  // deep void base\n"
    "        // Warm white streaks\n"
    "        col += vec3(0.9, 0.85, 0.75) * intensity * 0.4 * speed;\n"
    "        // Blue-shift streaks\n"
    "        col += vec3(0.3, 0.4, 0.8) * intensity * 0.15 * speed;\n"
    "    }\n"
    "\n"
    "    finalColor = vec4(col, 1.0);\n"
    "}\n";

EVSkybox LoadEVSkybox(void) {
    EVSkybox sky = {0};
    sky.shader = LoadShaderFromMemory(skybox_vs, skybox_fs);
    if (sky.shader.id == 0) {
        printf("[EV] WARNING: Skybox shader failed to compile\n");
        return sky;
    }
    sky.timeLoc = GetShaderLocation(sky.shader, "time");
    sky.skyTypeLoc = GetShaderLocation(sky.shader, "skyType");
    sky.resolutionLoc = GetShaderLocation(sky.shader, "resolution");
    sky.viewMatrixLoc = GetShaderLocation(sky.shader, "viewMatrix");
    sky.param1Loc = GetShaderLocation(sky.shader, "param1");
    sky.param2Loc = GetShaderLocation(sky.shader, "param2");
    sky.ready = true;
    printf("[EV] Skybox shader loaded\n");
    return sky;
}

void UnloadEVSkybox(EVSkybox *sky) {
    if (sky->ready) {
        UnloadShader(sky->shader);
        sky->ready = false;
    }
}

void DrawSkybox(EVSkybox *sky, Camera3D camera, SkyType type, float time, float param1, float param2) {
    if (!sky->ready || type == SKY_NONE) return;

    int sky_type_int = (int)type;
    float res[2] = {(float)RENDER_W, (float)RENDER_H};

    // Build view matrix from camera
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);

    SetShaderValue(sky->shader, sky->timeLoc, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sky->shader, sky->skyTypeLoc, &sky_type_int, SHADER_UNIFORM_INT);
    SetShaderValue(sky->shader, sky->resolutionLoc, res, SHADER_UNIFORM_VEC2);
    SetShaderValueMatrix(sky->shader, sky->viewMatrixLoc, view);
    SetShaderValue(sky->shader, sky->param1Loc, &param1, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sky->shader, sky->param2Loc, &param2, SHADER_UNIFORM_FLOAT);

    // Draw full-screen quad with depth test/write disabled
    rlDisableDepthTest();
    rlDisableDepthMask();
    BeginShaderMode(sky->shader);
    DrawRectangle(0, 0, RENDER_W, RENDER_H, WHITE);
    EndShaderMode();
    rlEnableDepthMask();
    rlEnableDepthTest();
}
