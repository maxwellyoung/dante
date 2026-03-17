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
    "    vec2 texelSize = 1.0 / vec2(512.0);\n"
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
    "        // CONCRETE — subtle pitting and surface variation\n"
    "        float n = fbm(surfUV * 3.0);\n"
    "        baseColor *= 0.92 + n * 0.16;\n"
    "        specMult = 0.08;\n"
    "    }\n"
    "    else if (materialId == 1) {\n"
    "        // MARBLE — veined with Voronoi cellular structure\n"
    "        float vein = sin(surfUV.x * 4.0 + surfUV.y * 2.0 + sin(wp.z * 3.0) * 2.0);\n"
    "        vein = smoothstep(0.0, 0.15, abs(vein));\n"
    "        float n = fbm(surfUV * 5.0);\n"
    "        // Voronoi adds organic cellular veining — natural stone character\n"
    "        float cell = voronoi(surfUV * 3.0);\n"
    "        float vein2 = smoothstep(0.02, 0.08, cell) * 0.15;\n"
    "        baseColor *= mix(0.80, 1.0, vein) + n * 0.06 + vein2;\n"
    "        specMult = 0.35;\n"  // polished
    "    }\n"
    "    else if (materialId == 2) {\n"
    "        // WOOD — grain lines along one axis\n"
    "        float grain = sin(surfUV.y * 12.0 + noise(surfUV * 4.0) * 3.0) * 0.5 + 0.5;\n"
    "        grain = smoothstep(0.3, 0.7, grain);\n"
    "        float ring = noise(vec2(surfUV.x * 1.5, surfUV.y * 8.0));\n"
    "        baseColor *= 0.85 + grain * 0.12 + ring * 0.06;\n"
    "        specMult = 0.12;\n"
    "    }\n"
    "    else if (materialId == 3) {\n"
    "        // CARPET — dense fuzzy noise, matte\n"
    "        float n = fbm(surfUV * 8.0);\n"
    "        baseColor *= 0.88 + n * 0.18;\n"
    "        specMult = 0.0;\n"  // fully matte
    "    }\n"
    "    else if (materialId == 4) {\n"
    "        // WALLPAPER — damask: repeating pattern + subtle color shift\n"
    "        vec2 tile = fract(surfUV * 2.0);\n"
    "        float pattern = sin(tile.x * 6.283) * sin(tile.y * 6.283);\n"
    "        pattern = smoothstep(-0.2, 0.2, pattern);\n"
    "        float n = noise(surfUV * 6.0);\n"
    "        baseColor *= 0.92 + pattern * 0.08 + n * 0.04;\n"
    "        specMult = 0.05;\n"
    "    }\n"
    "    else if (materialId == 5) {\n"
    "        // TILE — grid with grout lines\n"
    "        vec2 tileUV = fract(surfUV * 3.0);\n"
    "        float grout = step(0.06, tileUV.x) * step(0.06, tileUV.y);\n"
    "        baseColor *= mix(0.65, 1.0, grout);\n"
    "        specMult = 0.25;\n"  // glazed tile
    "    }\n"
    "    else if (materialId == 6) {\n"
    "        // BRASS — metallic shimmer with subtle patina tarnish\n"
    "        float n = noise(surfUV * 20.0);\n"
    "        // Patina: Voronoi-based tarnish in crevices — aged luxury\n"
    "        float patina = voronoi(surfUV * 8.0);\n"
    "        float tarnish = smoothstep(0.0, 0.3, patina) * 0.12;\n"
    "        baseColor *= 0.93 + n * 0.08 + tarnish;\n"
    "        // Patina shifts hue slightly green in dark areas\n"
    "        baseColor.g += (1.0 - patina) * 0.015;\n"
    "        specMult = 0.55 + patina * 0.15;\n"  // shinier where not tarnished
    "    }\n"
    "    else if (materialId == 7) {\n"
    "        // GLASS — subtle reflection, slight transparency feel\n"
    "        float n = noise(surfUV * 30.0);\n"
    "        baseColor *= 0.98 + n * 0.04;\n"
    "        specMult = 0.5;\n"
    "    }\n"
    "    else if (materialId == 8) {\n"
    "        // LEATHER — subtle grain, warm\n"
    "        float n = fbm(surfUV * 10.0);\n"
    "        float grain = noise(surfUV * 25.0);\n"
    "        baseColor *= 0.90 + n * 0.08 + grain * 0.04;\n"
    "        specMult = 0.18;\n"
    "    }\n"
    "    else if (materialId == 9) {\n"
    "        // FABRIC — soft weave pattern\n"
    "        vec2 weave = fract(surfUV * 6.0);\n"
    "        float warp = step(0.5, weave.x);\n"
    "        float weft = step(0.5, weave.y);\n"
    "        float pattern = abs(warp - weft) * 0.08;\n"
    "        float n = noise(surfUV * 12.0);\n"
    "        baseColor *= 0.92 + pattern + n * 0.06;\n"
    "        specMult = 0.02;\n"
    "    }\n"
    "    else if (materialId == 10) {\n"
    "        // CHECKERBOARD — two-tone floor from a single plane\n"
    "        vec2 checker = floor(surfUV * 2.0);\n"
    "        float check = mod(checker.x + checker.y, 2.0);\n"
    "        baseColor *= mix(0.78, 1.0, check);\n"
    "        // Per-tile noise variation\n"
    "        float tileNoise = noise(checker * 7.3) * 0.04;\n"
    "        baseColor += tileNoise;\n"
    "        specMult = 0.3;\n"
    "    }\n"
    "    else if (materialId == 11) {\n"
    "        // HERRINGBONE — interlocking plank pattern\n"
    "        vec2 uv = surfUV * 3.0;\n"
    "        float row = floor(uv.y);\n"
    "        float shifted = uv.x + mod(row, 2.0) * 0.5;\n"
    "        float plank = mod(floor(shifted), 2.0);\n"
    "        // Grain runs along plank length, alternating direction\n"
    "        float grainDir = (plank > 0.5) ? uv.x : uv.y;\n"
    "        float grain = sin(grainDir * 20.0 + noise(surfUV * 5.0) * 3.0) * 0.5 + 0.5;\n"
    "        grain = smoothstep(0.3, 0.7, grain);\n"
    "        baseColor *= mix(0.82, 1.0, plank) + grain * 0.06;\n"
    "        // Grout between planks\n"
    "        vec2 plankUV = fract(vec2(shifted, uv.y));\n"
    "        float grout = step(0.04, plankUV.x) * step(0.04, plankUV.y);\n"
    "        baseColor *= mix(0.7, 1.0, grout);\n"
    "        specMult = 0.15;\n"
    "    }\n"
    "    else if (materialId == 12) {\n"
    "        // PARQUET — alternating grain direction in tiles\n"
    "        vec2 tile = floor(surfUV * 2.5);\n"
    "        float dir = mod(tile.x + tile.y, 2.0);\n"
    "        vec2 localUV = fract(surfUV * 2.5);\n"
    "        float grainCoord = (dir > 0.5) ? localUV.x : localUV.y;\n"
    "        float grain = sin(grainCoord * 25.0 + noise(surfUV * 6.0) * 2.0) * 0.5 + 0.5;\n"
    "        baseColor *= 0.9 + grain * 0.1;\n"
    "        // Tile border\n"
    "        float border = step(0.03, localUV.x) * step(0.03, localUV.y)\n"
    "                      * step(0.03, 1.0-localUV.x) * step(0.03, 1.0-localUV.y);\n"
    "        baseColor *= mix(0.75, 1.0, border);\n"
    "        specMult = 0.14;\n"
    "    }\n"
    "    else if (materialId == 13) {\n"
    "        // VELVET — directional sheen, view-dependent luminance\n"
    "        // Velvet appears lighter at grazing angles (Fresnel-like nap)\n"
    "        float NdotV = max(dot(norm, normalize(viewPos - fragPosition)), 0.0);\n"
    "        float sheen = pow(1.0 - NdotV, 2.0) * 0.25;\n"
    "        float nap = noise(surfUV * 15.0) * 0.06;\n"
    "        baseColor *= 0.88 + sheen + nap;\n"
    "        specMult = 0.03;\n"  // nearly matte, except at edges
    "    }\n"
    "\n"
    "    // Shadow\n"
    "    float shadow = ShadowCalc(fragPosLightSpace);\n"
    "\n"
    "    // Key light — warm, generous, attenuated by shadow\n"
    "    float NdotL = dot(norm, -lightDir);\n"
    "    float halfLambert = NdotL * 0.5 + 0.5;\n"
    "    halfLambert = halfLambert * halfLambert;\n"
    "    vec3 keyDiffuse = lightColor * halfLambert * (1.0 - shadow * 0.7);\n"
    "\n"
    "    // Fill light — soft counter\n"
    "    float NdotF = dot(norm, -fillDir);\n"
    "    float fillLambert = max(NdotF * 0.5 + 0.5, 0.0);\n"
    "    vec3 fillDiffuse = fillColor * fillLambert * 0.5;\n"
    "\n"
    "    // Point lights — practicals (lamps, candles, fixtures) + floor pools\n"
    "    vec3 pointLit = vec3(0.0);\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "        if (pointRadius[i] > 0.0) {\n"
    "            vec3 toLight = pointPos[i] - fragPosition;\n"
    "            float pDist = length(toLight);\n"
    "            vec3 pDir = toLight / max(pDist, 0.001);\n"
    "            float NdotP = max(dot(norm, pDir), 0.0);\n"
    "            float atten = 1.0 / (1.0 + pDist * pDist / (pointRadius[i] * pointRadius[i] * 0.25));\n"
    "            atten *= smoothstep(pointRadius[i], pointRadius[i] * 0.5, pDist);\n"
    "            pointLit += pointColor[i] * NdotP * atten;\n"
    "            // Light pool on floors — soft projected circle beneath practicals\n"
    "            if (norm.y > 0.9) {\n"
    "                float floorDist = length(fragPosition.xz - pointPos[i].xz);\n"
    "                float poolR = pointRadius[i] * 0.25;\n"
    "                float pool = smoothstep(poolR, poolR * 0.2, floorDist);\n"
    "                pointLit += pointColor[i] * pool * 0.4 * atten;\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // Rim — subtle warm edge\n"
    "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
    "    float rim = 1.0 - max(dot(viewDir, norm), 0.0);\n"
    "    rim = pow(rim, 3.0) * 0.18;\n"
    "    vec3 rimColor = lightColor * rim;\n"
    "\n"
    "    // Specular — per-material intensity\n"
    "    vec3 halfDir = normalize(-lightDir + viewDir);\n"
    "    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);\n"
    "    vec3 specColor = lightColor * spec * specMult;\n"
    "\n"
    "    // Normal-based AO\n"
    "    float ao = 0.5 + 0.5 * max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);\n"
    "    ao = mix(0.65, 1.0, ao);\n"
    "\n"
    "    // Self-lit — bright surfaces glow softly\n"
    "    float brightness = dot(baseColor, vec3(0.299, 0.587, 0.114));\n"
    "    float selfLit = smoothstep(0.7, 0.9, brightness) * 0.5;\n"
    "\n"
    "    // Fresnel — edges catch more light\n"
    "    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0) * 0.08;\n"
    "\n"
    "    vec3 lit = baseColor * (ambient * ao * (1.0 - selfLit) + keyDiffuse + fillDiffuse + pointLit + selfLit)"
    "              + rimColor + specColor + vec3(fresnel);\n"
    "\n"
    "    // Fog — warm, not black\n"
    "    float fogDist = length(viewPos - fragPosition);\n"
    "    float fog = 1.0 - exp(-fogDensity * fogDist * fogDist);\n"
    "    fog = clamp(fog, 0.0, 1.0);\n"
    "    lit = mix(lit, fogColor, fog);\n"
    "    lit += fog * vec3(-0.01, 0.0, 0.02);\n"
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

void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity) {
    if (!lighting->ready) return;
    float viewPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lighting->shader, lighting->viewPosLoc, viewPos, SHADER_UNIFORM_VEC3);
    float fogCol[3] = {fogColor.r / 255.0f, fogColor.g / 255.0f, fogColor.b / 255.0f};
    SetShaderValue(lighting->shader, lighting->fogColorLoc, fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);
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
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.3f, -0.6f}),
        .keyColor = {1.8f, 1.3f, 0.85f},        // sodium streetlight — HOT orange, boosted
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.4f, 0.5f}),
        .fillColor = {0.5f, 0.52f, 0.7f},        // dashboard blue — boosted
        .ambient = {0.4f, 0.36f, 0.42f},          // see the leather, see the driver — boosted
        // Taxi meter glow — green-teal dashboard
        .pointPos = {{0.45f, 0.72f, -1.06f}},
        .pointColor = {{0.6f, 1.0f, 0.8f}},
        .pointRadius = {5.0f}
    };
}

SceneLighting LightingPreset_Exterior(void) {
    // Auckland at 2AM — moonlight gradient, not a directional sun
    // Easy Delivery Co. insight: ambient gradient > orbiting light.
    // Overcast night: everything lit by a single blue-grey gradient.
    // The hotel entrance warm spill is the CONTRAST that draws you in.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.2f, -0.7f, 0.3f}),
        .keyColor = {0.45f, 0.50f, 0.65f},       // moonlight — ambient gradient, not spotlight
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.4f, -0.6f}),
        .fillColor = {0.18f, 0.16f, 0.14f},      // ground bounce — subtle
        .ambient = {0.22f, 0.23f, 0.30f},         // higher ambient — overcast gradient feeling
        // Two practicals: lamppost + warm hotel entrance spill (safety vs night)
        .pointPos = {{-6.0f, 3.8f, -1.0f}, {0.0f, 2.0f, -4.0f}},
        .pointColor = {{1.0f, 0.80f, 0.50f}, {1.2f, 0.90f, 0.55f}},
        .pointRadius = {14.0f, 8.0f},
    };
}

SceneLighting LightingPreset_Lobby(void) {
    // Grand lobby — one dominant chandelier, deep shadows elsewhere
    // Wes Anderson symmetry: light pools on marble, dark wood walls
    // Steeper key angle + much lower ambient = contrast separation
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.9f, -0.4f}),
        .keyColor = {2.2f, 1.6f, 0.9f},          // hot chandelier — boosted for contrast
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.4f}),
        .fillColor = {0.04f, 0.06f, 0.03f},     // nearly invisible — green whisper
        .ambient = {0.03f, 0.025f, 0.02f},       // VERY LOW — walls go dark, light pools pop
        // Tight main chandelier + dim accents (tighter radii = more contrast)
        .pointPos = {{-2.0f, 6.4f, -2.0f}, {-4, 6.4f, 0}, {4, 6.4f, -3}, {0, 3.5f, -5}},
        .pointColor = {{2.0f, 1.5f, 0.85f}, {0.5f, 0.38f, 0.22f}, {0.5f, 0.38f, 0.22f}, {0.7f, 0.50f, 0.30f}},
        .pointRadius = {10.0f, 6.0f, 6.0f, 5.0f},
    };
}

SceneLighting LightingPreset_Elevator(void) {
    // Enclosed box — single overhead fluorescent, slightly green
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .keyColor = {0.85f, 0.90f, 0.80f},      // overhead fluorescent
        .fillDir = Vector3Normalize((Vector3){0.0f, 1.0f, 0.0f}),
        .fillColor = {0.15f, 0.14f, 0.12f},     // floor bounce
        .ambient = {0.15f, 0.15f, 0.14f},
        .pointPos = {{0, 2.4f, 0}},
        .pointColor = {{0.7f, 0.75f, 0.65f}},
        .pointRadius = {3.0f},
    };
}

SceneLighting LightingPreset_ElevatorSpace(void) {
    // Brass box in space — warm overhead panel, Earth blue from below
    // Through the glass floor: lobby glow fading, Earth glow rising
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .keyColor = {0.75f, 0.70f, 0.55f},      // warmer brass, less fluorescent
        .fillDir = Vector3Normalize((Vector3){-0.3f, 1.0f, 0.0f}),
        .fillColor = {0.12f, 0.18f, 0.28f},     // Earth blue from below/left
        .ambient = {0.10f, 0.11f, 0.14f},        // cooler ambient than terrestrial
        .pointPos = {{0, 2.4f, 0}, {-1.0f, -0.5f, 0}},
        .pointColor = {{0.6f, 0.55f, 0.40f}, {0.08f, 0.15f, 0.25f}},
        .pointRadius = {3.0f, 5.0f},
    };
}

SceneLighting LightingPreset_Hallway(void) {
    // Long corridor — overhead practicals at intervals, warm but dim
    // Think: Kubrick's Shining hallway, but warmer
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.85f, -0.3f}),
        .keyColor = {0.9f, 0.78f, 0.55f},       // warm overhead
        .fillDir = Vector3Normalize((Vector3){0.4f, 0.3f, 0.2f}),
        .fillColor = {0.18f, 0.16f, 0.13f},     // wall bounce
        .ambient = {0.12f, 0.11f, 0.10f},        // dim corridors
        // First ceiling light panel — player walks toward these
        .pointPos = {{0, 3.9f, -3.0f}},
        .pointColor = {{0.8f, 0.65f, 0.4f}},
        .pointRadius = {8.0f},
    };
}

SceneLighting LightingPreset_Room(void) {
    // The hotel room — the emotional core
    // Starts cool (arrived at night), warms as you settle in
    // Golden key from window side, cool fill from corridor
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.2f, -0.7f, -0.3f}),
        .keyColor = {1.2f, 1.05f, 0.82f},       // golden — Hotel Chevalier
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.30f, 0.35f, 0.45f},     // cool blue contrast
        .ambient = {0.10f, 0.09f, 0.08f},        // LOW — 2AM hotel room, shadows must read
        // Ceiling light panel — center of room
        .pointPos = {{0, 3.68f, 0}, {-2.5f, 0.85f, -3.8f}, {0, 0.5f, 3.5f}},
        .pointColor = {{0.9f, 0.7f, 0.45f}, {0.8f, 0.6f, 0.35f}, {0.5f, 0.4f, 0.2f}},
        .pointRadius = {7.0f, 4.0f, 3.0f},
    };
}

SceneLighting LightingPreset_Bathroom(void) {
    // Harsh overhead, clinical white, slight blue-green tinge
    // Mirror's Edge bathroom energy
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.3f, -0.9f, -0.2f}),
        .keyColor = {1.6f, 1.5f, 1.4f},        // harsh angled — casts side shadows on fixtures
        .fillDir = Vector3Normalize((Vector3){-0.3f, 0.5f, 0.2f}),
        .fillColor = {0.06f, 0.08f, 0.10f},     // minimal cool bounce
        .ambient = {0.05f, 0.06f, 0.07f},        // VERY LOW — stark shadows under basin, tub edge
        // Ando slot window — tight bright pool
        .pointPos = {{0, 2.6f, -1.88f}},
        .pointColor = {{1.2f, 1.15f, 1.1f}},
        .pointRadius = {3.0f},
    };
}

SceneLighting LightingPreset_Balcony(void) {
    // Orbital observation deck — Earth glow from below, starfield above
    // Easy Delivery Co. insight: no directional sun in space. Just gradients.
    // Earth glow (cool blue uplighting) vs. interior warmth (golden back wall).
    // The contrast IS the emotion — cold void, warm chair.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, 0.6f, -0.8f}),   // from Earth — uplighting
        .keyColor = {0.50f, 0.70f, 0.95f},       // cooler blue — Earth is vast, cold
        .fillDir = Vector3Normalize((Vector3){0.0f, -0.5f, 0.5f}),
        .fillColor = {0.40f, 0.30f, 0.18f},     // warm back wall — safety behind you
        .ambient = {0.22f, 0.24f, 0.34f},        // lowered — stronger contrast lit vs dark
        // Earth glow: massive reach. Interior lamp: intimate, warm, close.
        .pointPos = {{0.0f, -0.5f, -8.0f}, {0.0f, 1.5f, 3.0f}},
        .pointColor = {{0.5f, 0.75f, 1.0f}, {0.8f, 0.55f, 0.30f}},
        .pointRadius = {30.0f, 5.0f},
    };
}

SceneLighting LightingPreset_SpaceLobby(void) {
    // Grand station lobby — chandelier + elevator shaft glow
    // Cold starlight from observation window, warm brass interior
    // Key comes DOWN at an angle — overhead chandelier casts long shadows
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.8f, -0.4f}),   // steep overhead — drama
        .keyColor = {0.55f, 0.65f, 0.85f},        // Earth blue tint
        .fillDir = Vector3Normalize((Vector3){0.2f, 0.5f, 0.3f}),
        .fillColor = {0.12f, 0.18f, 0.28f},       // cool blue bounce — dim
        .ambient = {0.08f, 0.09f, 0.12f},          // LOW — dramatic pools of light
        // Chandelier above observation area + Earth glow on floor
        .pointPos = {{0, 6.4f, -3.0f}, {-8, 3, 0}, {8, 3, 0}, {0, 0.5f, -6.0f}},
        .pointColor = {{0.85f, 0.65f, 0.40f}, {0.3f, 0.45f, 0.65f}, {0.3f, 0.45f, 0.65f}, {0.4f, 0.6f, 0.9f}},
        .pointRadius = {14.0f, 10.0f, 10.0f, 12.0f},
    };
}

SceneLighting LightingPreset_SpaceCorridor(void) {
    // Narrow corridor — overhead amber strips, porthole starlight
    // Kubrick hallway but cold and blue-shifted
    // Strong overhead key for shadow drama along the walkway
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.9f, -0.2f}),   // steep overhead
        .keyColor = {1.2f, 1.0f, 0.70f},           // warm overhead amber — BRIGHT
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.0f}),
        .fillColor = {0.20f, 0.28f, 0.40f},        // porthole starlight — cool blue
        .ambient = {0.30f, 0.30f, 0.36f},           // Readable at 480x300 — drama via shadows not crushing
        // Ceiling panels spread along corridor + one blue porthole
        .pointPos = {{0, 3.2f, -8}, {0, 3.2f, 0}, {0, 3.2f, 8}, {0, 3.2f, 16}},
        .pointColor = {{1.2f, 0.95f, 0.65f}, {1.1f, 0.90f, 0.60f}, {1.0f, 0.85f, 0.55f}, {0.6f, 0.7f, 0.9f}},
        .pointRadius = {18.0f, 18.0f, 18.0f, 14.0f},
    };
}

SceneLighting LightingPreset_SpaceSuite(void) {
    // Space suite — Earth glow from window, warm lamps by bed
    // Key angled down from window — casts shadows across furniture
    // Intimate room: tight pools, dark corners, Earth as backdrop
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.5f, -0.2f}),  // diagonal from window — shadows!
        .keyColor = {0.65f, 0.78f, 0.95f},        // Earth blue — strong but not overwhelming
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.4f, 0.2f}),
        .fillColor = {0.20f, 0.16f, 0.10f},       // warm floor bounce — dim
        .ambient = {0.08f, 0.08f, 0.10f},          // LOW — luxury suite at night
        // Ceiling light above bed + bedside lamps + Earth glow from window
        .pointPos = {{0, 4.8f, -4.0f}, {-2.5f, 0.85f, -4.8f}, {2.5f, 0.85f, -4.8f}, {-6, 2, 0}},
        .pointColor = {{1.0f, 0.80f, 0.50f}, {0.9f, 0.7f, 0.4f}, {0.9f, 0.7f, 0.4f}, {0.3f, 0.5f, 0.8f}},
        .pointRadius = {10.0f, 5.0f, 5.0f, 8.0f},
    };
}

SceneLighting LightingPreset_Hyperspace(void) {
    // Wormhole tunnel — everything glows, brass rings catch light
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, 0.0f, -1.0f}),
        .keyColor = {1.2f, 1.0f, 0.75f},
        .fillDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .fillColor = {0.40f, 0.45f, 0.60f},
        .ambient = {0.35f, 0.32f, 0.40f},
        .pointPos = {{0, 0, -20.0f}},
        .pointColor = {{1.0f, 0.85f, 0.55f}},
        .pointRadius = {30.0f},
    };
}

SceneLighting LightingPreset_ParisDream(void) {
    // Hotel Chevalier golden hour — the OPPOSITE of space blue
    // Steeper key angle creates longer shadows, drives contrast ratio up
    // Reduced ambient so shadows actually go dark (was 0.22 — too bright, QA fail)
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.7f, -0.6f, -0.3f}),
        .keyColor = {2.0f, 1.4f, 0.65f},
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.20f, 0.15f, 0.08f},
        .ambient = {0.12f, 0.09f, 0.05f},
        .pointPos = {{0, 3.6f, 0}, {-2.5f, 0.85f, -3.8f}, {2.5f, 0.85f, -3.8f}},
        .pointColor = {{1.6f, 1.1f, 0.50f}, {1.0f, 0.75f, 0.40f}, {1.0f, 0.75f, 0.40f}},
        .pointRadius = {8.0f, 4.0f, 4.0f},
    };
}

SceneLighting LightingPreset_ReturnTaxi(void) {
    // Dawn Auckland — soft pink-gold, NOT sodium orange
    // Warmer and brighter than night taxi — city must be readable
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.5f, -0.4f}),
        .keyColor = {1.4f, 1.0f, 0.75f},            // boosted — dawn sun is strong
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.3f, 0.5f}),
        .fillColor = {0.35f, 0.30f, 0.42f},          // brighter fill — dawn sky bounce
        .ambient = {0.42f, 0.38f, 0.40f},             // up from 0.30 — city silhouettes need light
        .pointPos = {{0.45f, 0.72f, -1.06f}, {0, 0.5f, -3.0f}},
        .pointColor = {{0.4f, 0.6f, 0.5f}, {0.6f, 0.45f, 0.35f}},  // dash + dawn windshield glow
        .pointRadius = {4.0f, 8.0f},
    };
}
