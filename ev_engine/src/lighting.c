// lighting.c — Custom shader lighting for EV engine
// Per-scene presets. Warm key, cool fill, point light for practicals.
// Tadao Ando shadows. Hotel Chevalier golden hour. Godard moonlight.

#include "lighting.h"
#include "raymath.h"
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
    "out vec3 fragPosition;\n"
    "out vec3 fragNormal;\n"
    "out vec4 fragColor;\n"
    "out vec2 fragTexCoord;\n"
    "void main() {\n"
    "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragColor = vertexColor;\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *fs_source =
    "#version 330\n"
    "in vec3 fragPosition;\n"
    "in vec3 fragNormal;\n"
    "in vec4 fragColor;\n"
    "in vec2 fragTexCoord;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 ambient;\n"
    "uniform vec3 fogColor;\n"
    "uniform float fogDensity;\n"
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 fillDir;\n"
    "uniform vec3 fillColor;\n"
    "uniform vec3 pointPos;\n"
    "uniform vec3 pointColor;\n"
    "uniform float pointRadius;\n"
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
    "        // MARBLE — veined, polished\n"
    "        float vein = sin(surfUV.x * 4.0 + surfUV.y * 2.0 + sin(wp.z * 3.0) * 2.0);\n"
    "        vein = smoothstep(0.0, 0.15, abs(vein));\n"
    "        float n = fbm(surfUV * 5.0);\n"
    "        baseColor *= mix(0.82, 1.0, vein) + n * 0.06;\n"
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
    "        // BRASS — metallic shimmer, high specular\n"
    "        float n = noise(surfUV * 20.0);\n"
    "        baseColor *= 0.95 + n * 0.08;\n"
    "        specMult = 0.6;\n"
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
    "\n"
    "    // Key light — warm, generous\n"
    "    float NdotL = dot(norm, -lightDir);\n"
    "    float halfLambert = NdotL * 0.5 + 0.5;\n"
    "    halfLambert = halfLambert * halfLambert;\n"
    "    vec3 keyDiffuse = lightColor * halfLambert;\n"
    "\n"
    "    // Fill light — soft counter\n"
    "    float NdotF = dot(norm, -fillDir);\n"
    "    float fillLambert = max(NdotF * 0.5 + 0.5, 0.0);\n"
    "    vec3 fillDiffuse = fillColor * fillLambert * 0.5;\n"
    "\n"
    "    // Point light — practical (lamp, candle, fixture)\n"
    "    vec3 pointLit = vec3(0.0);\n"
    "    if (pointRadius > 0.0) {\n"
    "        vec3 toLight = pointPos - fragPosition;\n"
    "        float pDist = length(toLight);\n"
    "        vec3 pDir = toLight / max(pDist, 0.001);\n"
    "        float NdotP = max(dot(norm, pDir), 0.0);\n"
    "        float atten = 1.0 / (1.0 + pDist * pDist / (pointRadius * pointRadius * 0.25));\n"
    "        atten *= smoothstep(pointRadius, pointRadius * 0.5, pDist);\n"
    "        pointLit = pointColor * NdotP * atten;\n"
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
        lighting.pointPosLoc = GetShaderLocation(lighting.shader, "pointPos");
        lighting.pointColorLoc = GetShaderLocation(lighting.shader, "pointColor");
        lighting.pointRadiusLoc = GetShaderLocation(lighting.shader, "pointRadius");
        lighting.materialIdLoc = GetShaderLocation(lighting.shader, "materialId");
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

    float keyDir[3] = {preset.keyDir.x, preset.keyDir.y, preset.keyDir.z};
    SetShaderValue(lighting->shader, lighting->lightDirLoc, keyDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->lightColorLoc, preset.keyColor, SHADER_UNIFORM_VEC3);

    float fillDir[3] = {preset.fillDir.x, preset.fillDir.y, preset.fillDir.z};
    SetShaderValue(lighting->shader, lighting->fillDirLoc, fillDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fillColorLoc, preset.fillColor, SHADER_UNIFORM_VEC3);

    SetShaderValue(lighting->shader, lighting->ambientLoc, preset.ambient, SHADER_UNIFORM_VEC3);

    // Point light
    float pointPos[3] = {preset.pointPos.x, preset.pointPos.y, preset.pointPos.z};
    SetShaderValue(lighting->shader, lighting->pointPosLoc, pointPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointColorLoc, preset.pointColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointRadiusLoc, &preset.pointRadius, SHADER_UNIFORM_FLOAT);
}

void SetPointLight(EVLighting *lighting, float x, float y, float z,
                   float r, float g, float b, float radius) {
    if (!lighting->ready) return;
    float pos[3] = {x, y, z};
    float col[3] = {r, g, b};
    SetShaderValue(lighting->shader, lighting->pointPosLoc, pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointColorLoc, col, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointRadiusLoc, &radius, SHADER_UNIFORM_FLOAT);
}

void SetMaterialId(EVLighting *lighting, int materialId) {
    if (!lighting->ready) return;
    SetShaderValue(lighting->shader, lighting->materialIdLoc, &materialId, SHADER_UNIFORM_INT);
}

// ============================================================
// PER-SCENE LIGHTING PRESETS
// Each scene has its own emotional lighting. This is Hotel Chevalier.
// ============================================================

SceneLighting LightingPreset_Taxi(void) {
    // City light fragments through rain-streaked windows
    // Dim, fragmented, orange sodium vapor + blue dashboard
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.3f, -0.6f}),
        .keyColor = {0.9f, 0.65f, 0.4f},       // sodium streetlight orange — lifted
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.4f, 0.5f}),
        .fillColor = {0.2f, 0.22f, 0.35f},      // dashboard blue reflection
        .ambient = {0.14f, 0.12f, 0.15f},        // dark but readable
        // Taxi meter glow — green-teal dashboard
        .pointPos = {0.45f, 0.72f, -1.06f},
        .pointColor = {0.4f, 0.7f, 0.6f},
        .pointRadius = 3.0f
    };
}

SceneLighting LightingPreset_Exterior(void) {
    // Auckland at 2AM — moonlight blue-white, warm hotel entrance spill
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.8f, 0.2f}),
        .keyColor = {0.55f, 0.60f, 0.75f},       // moonlight — brighter
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.3f, -0.8f}),
        .fillColor = {0.15f, 0.12f, 0.10f},      // ground bounce
        .ambient = {0.12f, 0.13f, 0.18f},         // night but readable — see the tower
        // Left lamppost — warm spill near entrance
        .pointPos = {-6.0f, 3.8f, -1.0f},
        .pointColor = {1.0f, 0.8f, 0.5f},
        .pointRadius = 14.0f,
    };
}

SceneLighting LightingPreset_Lobby(void) {
    // Grand lobby — overhead chandeliers, warm marble reflection
    // Think: Wes Anderson hotel interior, symmetrical, golden
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.1f, -0.9f, -0.2f}),
        .keyColor = {1.3f, 1.1f, 0.85f},        // warm overhead chandelier — brighter
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.6f, 0.2f}),
        .fillColor = {0.35f, 0.30f, 0.24f},     // marble floor bounce — lifted
        .ambient = {0.25f, 0.24f, 0.22f},        // grand lobby should be welcoming
        // Hanging sphere light cluster — lobby ceiling
        .pointPos = {-2.0f, 6.4f, -2.0f},
        .pointColor = {1.0f, 0.85f, 0.55f},
        .pointRadius = 18.0f,
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
        .pointPos = {0, 2.4f, 0},
        .pointColor = {0.7f, 0.75f, 0.65f},
        .pointRadius = 3.0f,
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
        .pointPos = {0, 3.9f, -3.0f},
        .pointColor = {0.8f, 0.65f, 0.4f},
        .pointRadius = 8.0f,
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
        .ambient = {0.22f, 0.21f, 0.20f},        // Ando low ambient
        // Ceiling light panel — center of room
        .pointPos = {0, 3.68f, 0},
        .pointColor = {0.9f, 0.7f, 0.45f},
        .pointRadius = 7.0f,
    };
}

SceneLighting LightingPreset_Bathroom(void) {
    // Harsh overhead, clinical white, slight blue-green tinge
    // Mirror's Edge bathroom energy
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.1f}),
        .keyColor = {0.95f, 0.98f, 1.0f},       // clinical white
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.8f, -0.3f}),
        .fillColor = {0.20f, 0.22f, 0.25f},     // tile reflection — cool
        .ambient = {0.25f, 0.26f, 0.28f},        // brighter ambient — tiles reflect
        // Ando slot window — bright diffuse light
        .pointPos = {0, 2.6f, -1.88f},
        .pointColor = {0.9f, 0.92f, 0.95f},
        .pointRadius = 4.5f,
    };
}

SceneLighting LightingPreset_Balcony(void) {
    // Orbital observation deck — Earth glow from below, starfield above
    // Contemplative, expansive, melancholy beauty
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.4f, -0.6f, 0.3f}),
        .keyColor = {0.45f, 0.55f, 0.70f},      // Earth reflected light — blue
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.8f, -0.5f}),
        .fillColor = {0.25f, 0.20f, 0.12f},     // warm interior bounce from back wall
        .ambient = {0.14f, 0.15f, 0.22f},        // see the balcony furniture + railing
        // Earth atmosphere glow — below horizon
        .pointPos = {0.0f, -1.0f, -15.0f},
        .pointColor = {0.4f, 0.6f, 0.8f},
        .pointRadius = 30.0f,
    };
}

SceneLighting LightingPreset_SpaceLobby(void) {
    // Grand station lobby — chandelier + elevator shaft glow
    // Cold starlight from observation window, warm brass interior
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.8f, -0.4f}),
        .keyColor = {0.50f, 0.55f, 0.70f},       // starlight through window
        .fillDir = Vector3Normalize((Vector3){0.2f, 0.5f, 0.3f}),
        .fillColor = {0.15f, 0.22f, 0.35f},      // Earth glow — blue bounce
        .ambient = {0.16f, 0.17f, 0.22f},         // grand lobby — visible
        // Chandelier above observation area
        .pointPos = {0, 6.4f, -3.0f},
        .pointColor = {0.85f, 0.65f, 0.40f},
        .pointRadius = 14.0f,
    };
}

SceneLighting LightingPreset_SpaceCorridor(void) {
    // Narrow corridor — overhead amber strips, porthole starlight
    // Kubrick hallway energy but cold and blue-shifted
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.9f, -0.2f}),
        .keyColor = {0.80f, 0.68f, 0.50f},       // warm overhead amber — brighter
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.0f}),
        .fillColor = {0.15f, 0.18f, 0.28f},      // porthole starlight — cool
        .ambient = {0.18f, 0.18f, 0.22f},         // navigable corridor
        // Ceiling light panel — player walks toward these
        .pointPos = {0, 3.4f, 0},
        .pointColor = {0.8f, 0.65f, 0.45f},
        .pointRadius = 10.0f,
    };
}

SceneLighting LightingPreset_SpaceSuite(void) {
    // Space suite — Earth glow from left window, warm lamp from bed area
    // The Glass Elevator room: luxury + void
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.6f, -0.5f, -0.2f}),
        .keyColor = {0.55f, 0.65f, 0.80f},       // Earth glow — from window, lifted
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.4f, 0.2f}),
        .fillColor = {0.25f, 0.22f, 0.18f},      // warm floor bounce
        .ambient = {0.22f, 0.22f, 0.26f},         // liveable — see the furniture
        // Dropped ceiling light above bed
        .pointPos = {0, 4.8f, -4.0f},
        .pointColor = {0.90f, 0.70f, 0.45f},
        .pointRadius = 12.0f,
    };
}
