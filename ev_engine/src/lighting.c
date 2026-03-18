// lighting.c — Custom shader lighting for EV engine
// Per-scene presets. Warm key, cool fill, point light for practicals.
// Tadao Ando shadows. Hotel Chevalier golden hour. Godard moonlight.

#include "lighting.h"
#include "raymath.h"
#include "rlgl.h"
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#endif
#include <stdio.h>

static const char *vs_source =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 vertexNormal;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "out vec3 fragPosition;\n"
    "out vec3 fragNormal;\n"
    "out vec4 fragColor;\n"
    "out vec2 fragTexCoord;\n"
    "out vec4 fragPosLightSpace;\n"
    "void main() {\n"
    "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragColor = vertexColor;\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragPosLightSpace = lightSpaceMatrix * vec4(fragPosition, 1.0);\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *fs_source =
    "#version 330\n"
    "in vec3 fragPosition;\n"
    "in vec3 fragNormal;\n"
    "in vec4 fragColor;\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragPosLightSpace;\n"
    "uniform sampler2D shadowMap;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 ambient;\n"
    "uniform vec3 fogColor;\n"
    "uniform float fogDensity;\n"
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 fillDir;\n"
    "uniform vec3 fillColor;\n"
    "uniform vec3 pointPos[4];\n"
    "uniform vec3 pointColor[4];\n"
    "uniform float pointRadius[4];\n"
    "uniform int materialId;\n"
    "uniform float time;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "\n"
    // === Noise functions for procedural textures ===
    "float hash(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.x + p3.y) * p3.z);\n"
    "}\n"
    "float noise(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    f = f * f * (3.0 - 2.0 * f);\n"  // smoothstep
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
    "    return v;\n"
    "}\n"
    "// Voronoi — cellular noise for organic patterns\n"
    "float voronoi(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    float minDist = 1.0;\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            vec2 neighbor = vec2(float(x), float(y));\n"
    "            vec2 point = vec2(hash(i + neighbor), hash(i + neighbor + 17.3));\n"
    "            float d = length(neighbor + point - f);\n"
    "            minDist = min(minDist, d);\n"
    "        }\n"
    "    }\n"
    "    return minDist;\n"
    "}\n"
    "\n"
    // Shadow calculation — PCF 3x3 for soft edges at 480x300
    "float ShadowCalc(vec4 lsPos) {\n"
    "    vec3 proj = lsPos.xyz / lsPos.w;\n"
    "    proj = proj * 0.5 + 0.5;\n"
    "    if (proj.z > 1.0) return 0.0;\n"
    "    float bias = 0.005;\n"
    "    float shadow = 0.0;\n"
    "    vec2 texelSize = 1.0 / vec2(2048.0);\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            float d = texture(shadowMap, proj.xy + vec2(x,y)*texelSize).r;\n"
    "            shadow += proj.z - bias > d ? 1.0 : 0.0;\n"
    "        }\n"
    "    }\n"
    "    return shadow / 9.0;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec4 texColor = texture(texture0, fragTexCoord);\n"
    "    vec3 baseColor = texColor.rgb * colDiffuse.rgb * fragColor.rgb;\n"
    "    vec3 norm = normalize(fragNormal);\n"
    "\n"
    // === Procedural surface detail — modulates baseColor before lighting ===
    // Pick world-space UV based on dominant normal axis (wall vs floor vs ceiling)
    "    vec3 wp = fragPosition;\n"
    "    vec3 an = abs(norm);\n"
    "    vec2 surfUV;\n"
    "    if (an.y > an.x && an.y > an.z) surfUV = wp.xz;\n"  // floor/ceiling
    "    else if (an.x > an.z) surfUV = wp.yz;\n"              // X-facing wall
    "    else surfUV = wp.xy;\n"                                // Z-facing wall
    "\n"
    // Per-material specular multiplier (overridden below)
    "    float specMult = 0.15;\n"
    "\n"
    "    if (materialId == 0) {\n"
    "        // CONCRETE — one noise layer, subtle variation\n"
    "        float n = noise(surfUV * 2.0);\n"
    "        baseColor *= 0.95 + n * 0.08;\n"
    "        specMult = 0.05;\n"
    "    }\n"
    "    else if (materialId == 1) {\n"
    "        // MARBLE — single vein pattern, polished\n"
    "        float vein = sin(surfUV.x * 4.0 + surfUV.y * 2.0 + noise(surfUV * 2.0) * 2.0);\n"
    "        vein = smoothstep(0.0, 0.12, abs(vein));\n"
    "        baseColor *= mix(0.85, 1.0, vein);\n"
    "        specMult = 0.4;\n"
    "    }\n"
    "    else if (materialId == 2) {\n"
    "        // WOOD — clean grain, one direction\n"
    "        float grain = sin(surfUV.y * 12.0 + noise(surfUV * 3.0) * 2.0) * 0.5 + 0.5;\n"
    "        grain = smoothstep(0.35, 0.65, grain);\n"
    "        baseColor *= 0.90 + grain * 0.10;\n"
    "        specMult = 0.12;\n"
    "    }\n"
    "    else if (materialId == 3) {\n"
    "        // CARPET — low-frequency variation, NOT noise. Warm matte.\n"
    "        float n = noise(surfUV * 1.5);\n"
    "        baseColor *= 0.94 + n * 0.08;\n"
    "        specMult = 0.02;\n"
    "    }\n"
    "    else if (materialId == 4) {\n"
    "        // WALLPAPER — visible damask repeat at 960x600\n"
    "        vec2 tile = fract(surfUV * 1.0);\n"
    "        float pattern = sin(tile.x * 6.283) * sin(tile.y * 6.283);\n"
    "        pattern = smoothstep(-0.3, 0.3, pattern);\n"
    "        baseColor *= 0.93 + pattern * 0.10;\n"
    "        specMult = 0.06;\n"
    "    }\n"
    "    else if (materialId == 5) {\n"
    "        // TILE — soft grout lines, not binary\n"
    "        vec2 tileUV = fract(surfUV * 3.0);\n"
    "        float grout = smoothstep(0.03, 0.08, tileUV.x) * smoothstep(0.03, 0.08, tileUV.y);\n"
    "        baseColor *= mix(0.80, 1.0, grout);\n"
    "        specMult = 0.25;\n"
    "    }\n"
    "    else if (materialId == 6) {\n"
    "        // BRASS — clean metallic, high specular\n"
    "        float n = noise(surfUV * 8.0);\n"
    "        baseColor *= 0.96 + n * 0.06;\n"
    "        specMult = 0.6;\n"
    "    }\n"
    "    else if (materialId == 7) {\n"
    "        // GLASS — near-perfect surface, very high specular\n"
    "        specMult = 0.55;\n"
    "    }\n"
    "    else if (materialId == 8) {\n"
    "        // LEATHER — subtle grain only\n"
    "        float n = noise(surfUV * 6.0);\n"
    "        baseColor *= 0.94 + n * 0.08;\n"
    "        specMult = 0.18;\n"
    "    }\n"
    "    else if (materialId == 9) {\n"
    "        // FABRIC — visible weave at render resolution\n"
    "        vec2 weave = fract(surfUV * 4.0);\n"
    "        float warp = step(0.5, weave.x);\n"
    "        float weft = step(0.5, weave.y);\n"
    "        float pattern = abs(warp - weft) * 0.10;\n"
    "        baseColor *= 0.93 + pattern;\n"
    "        specMult = 0.03;\n"
    "    }\n"
    "    else if (materialId == 10) {\n"
    "        // CHECKERBOARD — clean two-tone, slight per-tile warmth\n"
    "        vec2 checker = floor(surfUV * 2.0);\n"
    "        float check = mod(checker.x + checker.y, 2.0);\n"
    "        baseColor *= mix(0.75, 1.0, check);\n"
    "        specMult = 0.3;\n"
    "    }\n"
    "    else if (materialId == 11) {\n"
    "        // HERRINGBONE — plank pattern with alternating tone\n"
    "        vec2 uv = surfUV * 3.0;\n"
    "        float row = floor(uv.y);\n"
    "        float shifted = uv.x + mod(row, 2.0) * 0.5;\n"
    "        float plank = mod(floor(shifted), 2.0);\n"
    "        baseColor *= mix(0.85, 1.0, plank);\n"
    "        // Soft grout\n"
    "        vec2 plankUV = fract(vec2(shifted, uv.y));\n"
    "        float grout = smoothstep(0.02, 0.06, plankUV.x) * smoothstep(0.02, 0.06, plankUV.y);\n"
    "        baseColor *= mix(0.78, 1.0, grout);\n"
    "        specMult = 0.15;\n"
    "    }\n"
    "    else if (materialId == 12) {\n"
    "        // PARQUET — alternating tile direction\n"
    "        vec2 tile = floor(surfUV * 2.5);\n"
    "        float dir = mod(tile.x + tile.y, 2.0);\n"
    "        vec2 localUV = fract(surfUV * 2.5);\n"
    "        float grainCoord = (dir > 0.5) ? localUV.x : localUV.y;\n"
    "        float grain = sin(grainCoord * 20.0) * 0.5 + 0.5;\n"
    "        baseColor *= 0.92 + grain * 0.08;\n"
    "        // Soft border\n"
    "        float border = smoothstep(0.02, 0.05, localUV.x) * smoothstep(0.02, 0.05, localUV.y)\n"
    "                      * smoothstep(0.02, 0.05, 1.0-localUV.x) * smoothstep(0.02, 0.05, 1.0-localUV.y);\n"
    "        baseColor *= mix(0.80, 1.0, border);\n"
    "        specMult = 0.14;\n"
    "    }\n"
    "    else if (materialId == 13) {\n"
    "        // VELVET — view-dependent sheen, no noise\n"
    "        float NdotV = max(dot(norm, normalize(viewPos - fragPosition)), 0.0);\n"
    "        float sheen = pow(1.0 - NdotV, 2.0) * 0.2;\n"
    "        baseColor *= 0.90 + sheen;\n"
    "        specMult = 0.04;\n"
    "    }\n"
    "    else if (materialId == 14) {\n"
    "        // WATER — dark reflective surface with animated ripple\n"
    "        // Slow sine ripple distorts the normal for moving specular highlights\n"
    "        float ripple1 = sin(wp.x * 1.5 + time * 0.4) * cos(wp.z * 2.0 + time * 0.3);\n"
    "        float ripple2 = sin(wp.x * 3.0 - time * 0.5 + 1.7) * cos(wp.z * 1.2 + time * 0.2);\n"
    "        float ripple = (ripple1 + ripple2 * 0.5) * 0.08;\n"
    "        // Perturb normal — creates moving specular on a flat surface\n"
    "        norm = normalize(norm + vec3(ripple, 0.0, ripple * 0.7));\n"
    "        // Dark base — near-black with slight blue shift\n"
    "        baseColor = baseColor * 0.3 + vec3(0.01, 0.02, 0.04);\n"
    "        specMult = 0.7;\n"
    "    }\n"
    "    else if (materialId == 15) {\n"
    "        // PUDDLE — thin wet film on ground, picks up nearby light\n"
    "        // Subtle noise-based distortion for wet sheen, no animation\n"
    "        float wet = noise(surfUV * 3.0);\n"
    "        float edge = smoothstep(0.3, 0.7, wet);\n"
    "        // Darken the ground color, add slight reflective sheen\n"
    "        baseColor = baseColor * 0.25 + vec3(0.01, 0.015, 0.025);\n"
    "        // Fresnel-like view-dependent reflection on the puddle\n"
    "        float NdotV = max(dot(norm, normalize(viewPos - fragPosition)), 0.0);\n"
    "        float pudFresnel = pow(1.0 - NdotV, 4.0) * 0.4;\n"
    "        baseColor += vec3(pudFresnel) * edge;\n"
    "        specMult = 0.55;\n"
    "    }\n"
    "\n"
    "    // Shadow\n"
    "    float shadow = ShadowCalc(fragPosLightSpace);\n"
    "\n"
    "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
    "    float viewDot = max(dot(norm, viewDir), 0.0);\n"
    "\n"
    "    // ── Key light — Half-Lambert wrap (Valve's TF2 technique) ──\n"
    "    // Wraps light around surfaces instead of hard cutoff at 90 degrees.\n"
    "    // This is THE fix for flat-looking geometry.\n"
    "    float NdotL = dot(norm, -lightDir);\n"
    "    float halfLambert = NdotL * 0.5 + 0.5;\n"
    "    halfLambert = halfLambert * halfLambert;\n"  // square for softer falloff
    "    vec3 keyDiffuse = lightColor * halfLambert * (1.0 - shadow * 0.7);\n"
    "\n"
    "    // ── Fill light — Half-Lambert wrap, stronger contribution ──\n"
    "    // Fill gives dimensionality to the shadow side. Was 0.4x, now 0.7x.\n"
    "    float NdotF = dot(norm, -fillDir);\n"
    "    float fillHalf = NdotF * 0.5 + 0.5;\n"
    "    fillHalf = fillHalf * fillHalf;\n"
    "    vec3 fillDiffuse = fillColor * fillHalf * 0.7;\n"
    "\n"
    "    // ── Point lights — practicals with wrap lighting ──\n"
    "    vec3 pointLit = vec3(0.0);\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "        if (pointRadius[i] > 0.0) {\n"
    "            vec3 toLight = pointPos[i] - fragPosition;\n"
    "            float pDist = length(toLight);\n"
    "            vec3 pDir = toLight / max(pDist, 0.001);\n"
    "            float NdotP = dot(norm, pDir) * 0.5 + 0.5;\n"  // half-Lambert for points too
    "            NdotP = NdotP * NdotP;\n"
    "            float atten = 1.0 / (1.0 + pDist * pDist / (pointRadius[i] * pointRadius[i] * 0.25));\n"
    "            atten *= smoothstep(pointRadius[i], pointRadius[i] * 0.5, pDist);\n"
    "            pointLit += pointColor[i] * NdotP * atten;\n"
    "            // Light pool on floors\n"
    "            if (norm.y > 0.9) {\n"
    "                float floorDist = length(fragPosition.xz - pointPos[i].xz);\n"
    "                float poolR = pointRadius[i] * 0.25;\n"
    "                float pool = smoothstep(poolR, poolR * 0.2, floorDist);\n"
    "                pointLit += pointColor[i] * pool * 0.4 * atten;\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // ── Rim light — edge catch, architectural silhouette ──\n"
    "    // Broader + stronger than before. Makes geometry pop from background.\n"
    "    float rim = 1.0 - viewDot;\n"
    "    rim = pow(rim, 2.5) * 0.30;\n"
    "    vec3 rimColor = lightColor * rim;\n"
    "\n"
    "    // ── Specular — per-material power + intensity ──\n"
    "    // Variable hardness: rough materials = broad soft highlight,\n"
    "    // smooth materials = tight sharp highlight.\n"
    "    float specPower = 32.0;\n"
    "    if (materialId == 6) specPower = 48.0;\n"       // brass — tight
    "    else if (materialId == 7) specPower = 64.0;\n"  // glass — very tight
    "    else if (materialId == 1) specPower = 48.0;\n"  // marble — polished
    "    else if (materialId == 2) specPower = 16.0;\n"  // wood — broad
    "    else if (materialId == 3) specPower = 4.0;\n"   // carpet — almost none
    "    else if (materialId == 9) specPower = 6.0;\n"   // fabric — almost none
    "    else if (materialId == 13) specPower = 8.0;\n"  // velvet — soft sheen
    "    else if (materialId == 8) specPower = 12.0;\n"  // leather — broad
    "    else if (materialId == 14) specPower = 64.0;\n" // water — tight moving highlight
    "    else if (materialId == 15) specPower = 48.0;\n" // puddle — sharp wet glint
    "    vec3 halfDir = normalize(-lightDir + viewDir);\n"
    "    float spec = pow(max(dot(norm, halfDir), 0.0), specPower);\n"
    "    vec3 specColor = lightColor * spec * specMult;\n"
    "\n"
    "    // ── Ambient Occlusion — multi-factor ──\n"
    "    // 1) Normal vs up — downward-facing surfaces darken\n"
    "    float aoUp = 0.5 + 0.5 * max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);\n"
    "    // 2) Height gradient — lower parts of rooms naturally darker\n"
    "    float aoHeight = smoothstep(-0.5, 4.0, fragPosition.y);\n"
    "    // 3) Crease detection — where surfaces meet at angles\n"
    "    //    View-space normal curvature approximation\n"
    "    float aoCrease = smoothstep(0.0, 0.3, viewDot);\n"
    "    float ao = mix(0.45, 1.0, aoUp * mix(0.7, 1.0, aoHeight) * mix(0.6, 1.0, aoCrease));\n"
    "\n"
    "    // Self-lit — only very bright surfaces glow (light fixtures, not walls)\n"
    "    float brightness = dot(baseColor, vec3(0.299, 0.587, 0.114));\n"
    "    float selfLit = smoothstep(0.85, 0.95, brightness) * 0.2;\n"
    "\n"
    "    // ── Fresnel — edges catch more light (architectural depth cue) ──\n"
    "    float fresnel = pow(1.0 - viewDot, 3.0) * 0.12;\n"
    "\n"
    "    vec3 lit = baseColor * (ambient * ao * (1.0 - selfLit) + keyDiffuse + fillDiffuse + pointLit + selfLit)"
    "              + rimColor + specColor + vec3(fresnel);\n"
    "\n"
    "    // Fog — warm, not black\n"
    "    float fogDist = length(viewPos - fragPosition);\n"
    "    float fog = 1.0 - exp(-fogDensity * fogDist * fogDist);\n"
    "    fog = clamp(fog, 0.0, 1.0);\n"
    "    lit = mix(lit, fogColor, fog);\n"
    "\n"
    "    finalColor = vec4(lit, texColor.a * colDiffuse.a * fragColor.a);\n"
    "}\n";

EVLighting LoadEVLighting(void) {
    EVLighting lighting = {0};
    lighting.shader = LoadShaderFromMemory(vs_source, fs_source);

    if (lighting.shader.id > 0) {
        lighting.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lighting.shader, "matModel");
        lighting.shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(lighting.shader, "mvp");
        lighting.shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lighting.shader, "viewPos");
        lighting.viewPosLoc = GetShaderLocation(lighting.shader, "viewPos");
        lighting.ambientLoc = GetShaderLocation(lighting.shader, "ambient");
        lighting.fogColorLoc = GetShaderLocation(lighting.shader, "fogColor");
        lighting.fogDensityLoc = GetShaderLocation(lighting.shader, "fogDensity");
        lighting.lightDirLoc = GetShaderLocation(lighting.shader, "lightDir");
        lighting.lightColorLoc = GetShaderLocation(lighting.shader, "lightColor");
        lighting.fillDirLoc = GetShaderLocation(lighting.shader, "fillDir");
        lighting.fillColorLoc = GetShaderLocation(lighting.shader, "fillColor");
        for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
            char name[32];
            snprintf(name, sizeof(name), "pointPos[%d]", i);
            lighting.pointPosLoc[i] = GetShaderLocation(lighting.shader, name);
            snprintf(name, sizeof(name), "pointColor[%d]", i);
            lighting.pointColorLoc[i] = GetShaderLocation(lighting.shader, name);
            snprintf(name, sizeof(name), "pointRadius[%d]", i);
            lighting.pointRadiusLoc[i] = GetShaderLocation(lighting.shader, name);
        }
        lighting.materialIdLoc = GetShaderLocation(lighting.shader, "materialId");
        lighting.timeLoc = GetShaderLocation(lighting.shader, "time");
        lighting.shadowMapLoc = GetShaderLocation(lighting.shader, "shadowMap");
        lighting.lightSpaceMatrixLoc = GetShaderLocation(lighting.shader, "lightSpaceMatrix");
        int matNormalLoc = GetShaderLocation(lighting.shader, "matNormal");
        lighting.shader.locs[SHADER_LOC_MATRIX_NORMAL] = matNormalLoc;

        // Apply default preset (room)
        SetSceneLighting(&lighting, LightingPreset_Room());

        lighting.ready = true;
        printf("[EV] Lighting loaded — per-scene presets ready\n");
    } else {
        printf("[EV] WARNING: Lighting shader failed\n");
        lighting.ready = false;
    }
    return lighting;
}

void UnloadEVLighting(EVLighting *lighting) {
    if (lighting->ready) { UnloadShader(lighting->shader); lighting->ready = false; }
}

void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity, float time) {
    if (!lighting->ready) return;
    float viewPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lighting->shader, lighting->viewPosLoc, viewPos, SHADER_UNIFORM_VEC3);
    float fogCol[3] = {fogColor.r / 255.0f, fogColor.g / 255.0f, fogColor.b / 255.0f};
    SetShaderValue(lighting->shader, lighting->fogColorLoc, fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lighting->shader, lighting->timeLoc, &time, SHADER_UNIFORM_FLOAT);
}

void SetSceneLighting(EVLighting *lighting, SceneLighting preset) {
    if (!lighting->ready) return;
    lighting->activePreset = preset;

    float keyDir[3] = {preset.keyDir.x, preset.keyDir.y, preset.keyDir.z};
    SetShaderValue(lighting->shader, lighting->lightDirLoc, keyDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->lightColorLoc, preset.keyColor, SHADER_UNIFORM_VEC3);

    float fillDir[3] = {preset.fillDir.x, preset.fillDir.y, preset.fillDir.z};
    SetShaderValue(lighting->shader, lighting->fillDirLoc, fillDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fillColorLoc, preset.fillColor, SHADER_UNIFORM_VEC3);

    SetShaderValue(lighting->shader, lighting->ambientLoc, preset.ambient, SHADER_UNIFORM_VEC3);

    // Point lights
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        float pPos[3] = {preset.pointPos[i].x, preset.pointPos[i].y, preset.pointPos[i].z};
        SetShaderValue(lighting->shader, lighting->pointPosLoc[i], pPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(lighting->shader, lighting->pointColorLoc[i], preset.pointColor[i], SHADER_UNIFORM_VEC3);
        SetShaderValue(lighting->shader, lighting->pointRadiusLoc[i], &preset.pointRadius[i], SHADER_UNIFORM_FLOAT);
    }
}

void SetPointLight(EVLighting *lighting, float x, float y, float z,
                   float r, float g, float b, float radius) {
    SetPointLightIdx(lighting, 0, x, y, z, r, g, b, radius);
}

void SetPointLightIdx(EVLighting *lighting, int index, float x, float y, float z,
                      float r, float g, float b, float radius) {
    if (!lighting->ready || index < 0 || index >= MAX_POINT_LIGHTS) return;
    float pos[3] = {x, y, z};
    float col[3] = {r, g, b};
    SetShaderValue(lighting->shader, lighting->pointPosLoc[index], pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointColorLoc[index], col, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointRadiusLoc[index], &radius, SHADER_UNIFORM_FLOAT);
}

void SetMaterialId(EVLighting *lighting, int materialId) {
    if (!lighting->ready) return;
    SetShaderValue(lighting->shader, lighting->materialIdLoc, &materialId, SHADER_UNIFORM_INT);
}

// ============================================================
// SHADOW MAPPING
// ============================================================

static const char *shadow_vs =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *shadow_fs =
    "#version 330\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    finalColor = vec4(1.0);\n"
    "}\n";

void CreateShadowMap(EVLighting *lighting) {
    if (lighting->shadowReady) return;

    // Load depth-only shader
    lighting->shadowShader = LoadShaderFromMemory(shadow_vs, shadow_fs);
    if (lighting->shadowShader.id == 0) {
        printf("[EV] WARNING: Shadow shader failed\n");
        return;
    }
    lighting->shadowMvpLoc = GetShaderLocation(lighting->shadowShader, "mvp");

    // Create FBO with depth texture
    lighting->shadowFBO = rlLoadFramebuffer();
    if (lighting->shadowFBO == 0) {
        printf("[EV] WARNING: Shadow FBO failed\n");
        return;
    }

    rlEnableFramebuffer(lighting->shadowFBO);

    lighting->shadowDepthTex = rlLoadTextureDepth(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, false);
    rlTextureParameters(lighting->shadowDepthTex, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(lighting->shadowDepthTex, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
    rlFramebufferAttach(lighting->shadowFBO, lighting->shadowDepthTex,
                        RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

    // NOTE: glDrawBuffer(GL_NONE) omitted — Apple's Metal-translated GL
    // doesn't isolate per-FBO draw buffer state, kills color writes globally.
    // Depth-only FBO works fine without it (no color attachment = no color writes).

    if (rlFramebufferComplete(lighting->shadowFBO)) {
        lighting->shadowReady = true;
        lighting->shadowPassRan = false;
        printf("[EV] Shadow map created — %dx%d depth texture\n", SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    } else {
        printf("[EV] WARNING: Shadow FBO incomplete\n");
        rlUnloadFramebuffer(lighting->shadowFBO);
        lighting->shadowFBO = 0;
    }

    rlDisableFramebuffer();
}

void DestroyShadowMap(EVLighting *lighting) {
    if (!lighting->shadowReady) return;
    rlUnloadTexture(lighting->shadowDepthTex);
    rlUnloadFramebuffer(lighting->shadowFBO);
    UnloadShader(lighting->shadowShader);
    lighting->shadowReady = false;
}

void UpdateShadowMatrix(EVLighting *lighting, Vector3 keyDir, Vector3 sceneCenter, float sceneRadius) {
    if (!lighting->ready) return;

    // Light camera — orthographic, looking at scene from key light direction
    Vector3 lightPos = Vector3Add(sceneCenter, Vector3Scale(Vector3Negate(keyDir), sceneRadius * 2.0f));
    Matrix lightView = MatrixLookAt(lightPos, sceneCenter, (Vector3){0, 1, 0});
    Matrix lightProj = MatrixOrtho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius,
                                    0.1f, sceneRadius * 4.0f);
    lighting->lightSpaceMatrix = MatrixMultiply(lightView, lightProj);

    // Upload to lighting shader
    SetShaderValueMatrix(lighting->shader, lighting->lightSpaceMatrixLoc, lighting->lightSpaceMatrix);
}

void BindShadowMap(EVLighting *lighting) {
    if (!lighting->shadowReady || !lighting->ready) return;
    // Bind shadow depth texture to texture unit 1
    rlActiveTextureSlot(1);
    rlEnableTexture(lighting->shadowDepthTex);
    int slot = 1;
    SetShaderValue(lighting->shader, lighting->shadowMapLoc, &slot, SHADER_UNIFORM_INT);
}

void UnbindShadowMap(void) {
    rlActiveTextureSlot(1);
    rlDisableTexture();
    rlActiveTextureSlot(0);
}

// ============================================================
// PER-SCENE LIGHTING PRESETS
// Each scene has its own emotional lighting. This is Hotel Chevalier.
// ============================================================

SceneLighting LightingPreset_Taxi(void) {
    // City light fragments through rain-streaked windows
    // Michael Mann night: dark but EVERY element reads
    // Half-Lambert note: pull back key ~30%, lower ambient — wrap does the work now
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.3f, -0.6f}),
        .keyColor = {1.2f, 0.9f, 0.58f},         // sodium streetlight — pulled back for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.4f, 0.5f}),
        .fillColor = {0.35f, 0.38f, 0.52f},       // dashboard blue
        .ambient = {0.18f, 0.16f, 0.20f},          // lower — half-Lambert wraps enough light
        // Taxi meter glow — green-teal dashboard
        .pointPos = {{0.45f, 0.72f, -1.06f}},
        .pointColor = {{0.6f, 1.0f, 0.8f}},
        .pointRadius = {5.0f}
    };
}

SceneLighting LightingPreset_Exterior(void) {
    // Auckland at 2AM — moonlight gradient, not a directional sun
    // Half-Lambert wraps the moonlight beautifully — pull it back
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.2f, -0.7f, 0.3f}),
        .keyColor = {0.30f, 0.34f, 0.45f},        // moonlight — moderated
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.4f, -0.6f}),
        .fillColor = {0.10f, 0.09f, 0.08f},       // ground bounce — subtle
        .ambient = {0.10f, 0.11f, 0.15f},          // lower — night should feel dark
        // lamppost + warm hotel entrance spill
        .pointPos = {{-6.0f, 3.8f, -1.0f}, {0.0f, 2.0f, -4.0f}},
        .pointColor = {{0.8f, 0.65f, 0.40f}, {1.0f, 0.75f, 0.45f}},
        .pointRadius = {14.0f, 8.0f},
    };
}

SceneLighting LightingPreset_Lobby(void) {
    // Grand lobby — one dominant chandelier, deep shadows elsewhere
    // Wes Anderson symmetry: light pools on marble, dark wood walls
    // Half-Lambert note: key pulled back 40%, ambient halved — wrap prevents pitch black
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.9f, -0.4f}),
        .keyColor = {1.4f, 1.0f, 0.55f},          // chandelier — pulled back, half-Lambert wraps it
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.4f}),
        .fillColor = {0.08f, 0.06f, 0.04f},       // warm counter — dimmer
        .ambient = {0.04f, 0.03f, 0.02f},          // VERY low — pools must pop
        // Tight pools — dark between fixtures
        .pointPos = {{-2.0f, 6.4f, -2.0f}, {-4, 6.4f, 0}, {4, 6.4f, -3}, {0, 3.5f, -5}},
        .pointColor = {{1.8f, 1.3f, 0.7f}, {0.3f, 0.22f, 0.12f}, {0.3f, 0.22f, 0.12f}, {0.45f, 0.32f, 0.18f}},
        .pointRadius = {7.0f, 5.0f, 5.0f, 4.0f},
    };
}

SceneLighting LightingPreset_Elevator(void) {
    // Enclosed brass box — warm amber overhead, dark marble floor absorbs
    // Hotel Chevalier golden cage. Not clinical fluorescent.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .keyColor = {0.60f, 0.50f, 0.35f},       // warm amber — pulled back for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.0f, 1.0f, 0.0f}),
        .fillColor = {0.05f, 0.04f, 0.03f},      // dark marble absorbs
        .ambient = {0.03f, 0.03f, 0.025f},        // VERY LOW — brass catches light, deep shadows
        .pointPos = {{0, 2.4f, 0}, {-0.9f, 1.2f, 0}},
        .pointColor = {{0.7f, 0.58f, 0.38f}, {0.10f, 0.15f, 0.25f}},
        .pointRadius = {3.5f, 2.5f},
    };
}

SceneLighting LightingPreset_ElevatorSpace(void) {
    // Brass box in space — warm overhead panel, Earth blue from below
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .keyColor = {0.48f, 0.44f, 0.35f},       // warm brass — pulled back
        .fillDir = Vector3Normalize((Vector3){-0.3f, 1.0f, 0.0f}),
        .fillColor = {0.08f, 0.12f, 0.18f},      // Earth blue from below
        .ambient = {0.05f, 0.06f, 0.08f},         // low — brass catches light
        .pointPos = {{0, 2.4f, 0}, {-1.0f, -0.5f, 0}},
        .pointColor = {{0.45f, 0.38f, 0.28f}, {0.06f, 0.10f, 0.18f}},
        .pointRadius = {3.0f, 5.0f},
    };
}

SceneLighting LightingPreset_Hallway(void) {
    // Long corridor — overhead practicals at intervals, warm but dim
    // Think: Kubrick's Shining hallway, but warmer
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.85f, -0.3f}),
        .keyColor = {0.6f, 0.52f, 0.36f},        // warm overhead — pulled back
        .fillDir = Vector3Normalize((Vector3){0.4f, 0.3f, 0.2f}),
        .fillColor = {0.10f, 0.09f, 0.07f},      // wall bounce — dimmer
        .ambient = {0.06f, 0.05f, 0.04f},         // low — corridor should feel enclosed
        // First ceiling light panel
        .pointPos = {{0, 3.9f, -3.0f}},
        .pointColor = {{0.6f, 0.48f, 0.3f}},
        .pointRadius = {8.0f},
    };
}

SceneLighting LightingPreset_Room(void) {
    // The hotel room — the emotional core
    // 2AM: strong overhead pool + bedside warmth vs dark corners
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.85f, -0.3f}),
        .keyColor = {1.0f, 0.82f, 0.52f},        // golden — pulled back for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.06f, 0.08f, 0.12f},      // cool blue night — subtler
        .ambient = {0.03f, 0.025f, 0.02f},        // VERY LOW — corners dark, wrap handles the rest
        // Tight ceiling pool + warm bedside lamps
        .pointPos = {{0, 3.68f, 0}, {-2.5f, 0.85f, -3.8f}, {2.5f, 0.85f, -3.8f}},
        .pointColor = {{0.9f, 0.65f, 0.38f}, {0.6f, 0.45f, 0.25f}, {0.6f, 0.45f, 0.25f}},
        .pointRadius = {6.0f, 3.5f, 3.5f},
    };
}

SceneLighting LightingPreset_Bathroom(void) {
    // Harsh overhead, clinical white, slight blue-green tinge
    // Mirror's Edge bathroom energy
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.3f, -0.9f, -0.2f}),
        .keyColor = {1.0f, 0.95f, 0.90f},       // harsh but pulled back — half-Lambert wraps enough
        .fillDir = Vector3Normalize((Vector3){-0.3f, 0.5f, 0.2f}),
        .fillColor = {0.04f, 0.05f, 0.06f},     // minimal cool bounce
        .ambient = {0.03f, 0.035f, 0.04f},       // very low — stark shadows
        // Ando slot window
        .pointPos = {{0, 2.6f, -1.88f}},
        .pointColor = {{0.8f, 0.78f, 0.75f}},
        .pointRadius = {3.0f},
    };
}

SceneLighting LightingPreset_Balcony(void) {
    // Orbital observation deck — Earth glow from below, starfield above
    // The contrast IS the emotion — cold void, warm chair.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, 0.6f, -0.8f}),    // from Earth — uplighting
        .keyColor = {0.32f, 0.45f, 0.62f},        // Earth blue — pulled back, wrap does the work
        .fillDir = Vector3Normalize((Vector3){0.0f, -0.5f, 0.5f}),
        .fillColor = {0.25f, 0.18f, 0.10f},       // warm back wall
        .ambient = {0.12f, 0.14f, 0.20f},          // moderate — space is never pitch black
        // Earth glow + interior lamp + floor accents
        .pointPos = {{0.0f, 0.5f, -4.0f}, {0.0f, 1.5f, 3.0f}, {-1.5f, 0.2f, -2.0f}, {1.5f, 0.2f, -2.0f}},
        .pointColor = {{0.4f, 0.6f, 0.85f}, {0.55f, 0.38f, 0.20f}, {0.2f, 0.35f, 0.5f}, {0.2f, 0.35f, 0.5f}},
        .pointRadius = {35.0f, 5.0f, 12.0f, 12.0f},
    };
}

SceneLighting LightingPreset_SpaceLobby(void) {
    // Grand station lobby — chandelier + elevator shaft glow
    // Cold starlight from observation window, warm brass interior
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.8f, -0.4f}),    // steep overhead
        .keyColor = {0.35f, 0.42f, 0.55f},         // Earth blue — pulled back
        .fillDir = Vector3Normalize((Vector3){0.2f, 0.5f, 0.3f}),
        .fillColor = {0.08f, 0.12f, 0.18f},        // cool blue bounce
        .ambient = {0.04f, 0.05f, 0.07f},           // LOW — dramatic pools
        // Chandelier + Earth glow on floor
        .pointPos = {{0, 6.4f, -3.0f}, {-8, 3, 0}, {8, 3, 0}, {0, 0.5f, -6.0f}},
        .pointColor = {{0.6f, 0.45f, 0.28f}, {0.2f, 0.30f, 0.45f}, {0.2f, 0.30f, 0.45f}, {0.3f, 0.45f, 0.65f}},
        .pointRadius = {14.0f, 10.0f, 10.0f, 12.0f},
    };
}

SceneLighting LightingPreset_SpaceCorridor(void) {
    // Narrow corridor — warm amber strips vs cold void blue from portholes
    // Kubrick hallway: warm pools at intervals, blue-black between
    // Half-Lambert note: wrap lighting + strong points = walls finally read
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.9f, -0.2f}),    // steep overhead
        .keyColor = {0.8f, 0.65f, 0.40f},            // amber — moderated for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.0f}),
        .fillColor = {0.12f, 0.16f, 0.28f},           // porthole starlight — cooler
        .ambient = {0.10f, 0.10f, 0.16f},              // moderate — hull must read but not wash
        // Amber ceiling panels + porthole accent
        .pointPos = {{0, 2.0f, 0}, {0, 2.0f, 6}, {0, 2.0f, 12}, {-2, 1.5f, 12}},
        .pointColor = {{1.0f, 0.8f, 0.45f}, {0.9f, 0.7f, 0.40f}, {0.8f, 0.65f, 0.38f}, {0.25f, 0.38f, 0.70f}},
        .pointRadius = {18.0f, 18.0f, 18.0f, 12.0f},
    };
}

SceneLighting LightingPreset_SpaceSuite(void) {
    // Space suite — warm amber interior vs cold Earth blue from window
    // Hotel Chevalier in orbit: golden lamplight, blue void outside
    // Half-Lambert note: Earth blue wraps beautifully now, pull intensities back
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.5f, -0.2f}),  // diagonal from window — shadows!
        .keyColor = {0.35f, 0.45f, 0.60f},        // Earth blue key — pulled back, wrap does the work
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.4f, 0.2f}),
        .fillColor = {0.16f, 0.12f, 0.07f},       // warmer floor bounce — amber undertone
        .ambient = {0.05f, 0.04f, 0.03f},          // low — pools must do the work, dark corners
        // Ceiling light above bed (warm) + bedside lamps (warmer) + Earth glow (cool)
        .pointPos = {{0, 4.8f, -4.0f}, {-2.5f, 0.85f, -4.8f}, {2.5f, 0.85f, -4.8f}, {-6, 2, 0}},
        .pointColor = {{0.8f, 0.60f, 0.35f}, {0.7f, 0.52f, 0.30f}, {0.7f, 0.52f, 0.30f}, {0.15f, 0.30f, 0.55f}},
        .pointRadius = {10.0f, 6.0f, 6.0f, 8.0f},
    };
}

SceneLighting LightingPreset_Hyperspace(void) {
    // Wormhole tunnel — rings glow, convergence point blazes
    // Multiple point lights along tunnel so rings are visible at every depth
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, 0.0f, -1.0f}),
        .keyColor = {1.0f, 0.8f, 0.55f},        // pulled back — half-Lambert wraps the glow
        .fillDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .fillColor = {0.30f, 0.35f, 0.45f},      // fill — moderate for half-Lambert
        .ambient = {0.22f, 0.20f, 0.26f},        // moderate — no dead black, wrap handles rest
        .pointPos = {{0, 0, -5.0f}, {0, 0, -15.0f}, {0, 0, -30.0f}, {0, 0, -50.0f}},
        .pointColor = {{1.2f, 1.0f, 0.65f}, {1.0f, 0.85f, 0.55f}, {0.8f, 0.7f, 0.45f}, {0.6f, 0.5f, 0.35f}},
        .pointRadius = {15.0f, 20.0f, 25.0f, 30.0f},
    };
}

SceneLighting LightingPreset_ParisDream(void) {
    // Hotel Chevalier golden hour — HOT light, DEEP shadows
    // The dream is saturated, contrasty — memory intensifies everything
    // Half-Lambert note: golden wraps beautifully but needs pull-back to keep contrast
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.7f, -0.3f}),
        .keyColor = {1.4f, 0.95f, 0.42f},        // golden — pulled back, still hot
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.05f, 0.04f, 0.02f},      // barely there
        .ambient = {0.03f, 0.025f, 0.015f},       // VERY LOW — dream shadows go deep
        .pointPos = {{0, 3.6f, 0}, {-2.5f, 0.85f, -3.8f}, {2.5f, 0.85f, -3.8f}},
        .pointColor = {{1.2f, 0.85f, 0.40f}, {0.7f, 0.50f, 0.28f}, {0.7f, 0.50f, 0.28f}},
        .pointRadius = {7.0f, 3.5f, 3.5f},
    };
}

SceneLighting LightingPreset_ReturnTaxi(void) {
    // Dawn Auckland — golden hour glow through windshield
    // Everything reads warmer and brighter than night. Circle closing.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.4f, -0.5f}),
        .keyColor = {1.2f, 0.85f, 0.55f},             // dawn golden — pulled back
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.3f, 0.5f}),
        .fillColor = {0.25f, 0.22f, 0.30f},           // sky bounce — lilac dawn
        .ambient = {0.25f, 0.22f, 0.20f},              // moderate — dawn but not washed
        .pointPos = {{0.45f, 0.72f, -1.06f}, {0, 0.5f, -3.0f}, {0, 1.0f, 2.0f}},
        .pointColor = {{0.5f, 0.7f, 0.5f}, {0.6f, 0.45f, 0.28f}, {0.7f, 0.52f, 0.32f}},
        .pointRadius = {4.0f, 8.0f, 12.0f},
    };
}
