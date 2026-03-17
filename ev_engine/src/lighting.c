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
    // Point light uniforms
    "uniform vec3 pointPos;\n"
    "uniform vec3 pointColor;\n"
    "uniform float pointRadius;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 texColor = texture(texture0, fragTexCoord);\n"
    "    vec3 baseColor = texColor.rgb * colDiffuse.rgb * fragColor.rgb;\n"
    "    vec3 norm = normalize(fragNormal);\n"
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
    "        float dist = length(toLight);\n"
    "        vec3 pointDir = toLight / max(dist, 0.001);\n"
    "        float NdotP = max(dot(norm, pointDir), 0.0);\n"
    "        // Smooth inverse-square falloff with radius cutoff\n"
    "        float atten = 1.0 / (1.0 + dist * dist / (pointRadius * pointRadius * 0.25));\n"
    "        atten *= smoothstep(pointRadius, pointRadius * 0.5, dist);\n"
    "        pointLit = pointColor * NdotP * atten;\n"
    "    }\n"
    "\n"
    "    // Rim — subtle warm edge\n"
    "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
    "    float rim = 1.0 - max(dot(viewDir, norm), 0.0);\n"
    "    rim = pow(rim, 3.0) * 0.18;\n"
    "    vec3 rimColor = lightColor * rim;\n"
    "\n"
    "    // Specular — Blinn-Phong, subtle for matte concrete and brass\n"
    "    vec3 halfDir = normalize(-lightDir + viewDir);\n"
    "    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);\n"
    "    vec3 specColor = lightColor * spec * 0.15;\n"
    "\n"
    "    // Normal-based AO: floors and upward faces brighter, undersides darker\n"
    "    float ao = 0.5 + 0.5 * max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);\n"
    "    ao = mix(0.65, 1.0, ao);\n"
    "\n"
    "    // Self-lit — bright surfaces (porcelain, light panels) glow softly\n"
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
    "    float dist = length(viewPos - fragPosition);\n"
    "    float fog = 1.0 - exp(-fogDensity * dist * dist);\n"
    "    fog = clamp(fog, 0.0, 1.0);\n"
    "    lit = mix(lit, fogColor, fog);\n"
    "    // Atmospheric perspective — slight cool shift at distance\n"
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

// ============================================================
// PER-SCENE LIGHTING PRESETS
// Each scene has its own emotional lighting. This is Hotel Chevalier.
// ============================================================

SceneLighting LightingPreset_Taxi(void) {
    // City light fragments through rain-streaked windows
    // Dim, fragmented, orange sodium vapor + blue dashboard
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.3f, -0.6f}),
        .keyColor = {0.8f, 0.55f, 0.3f},       // sodium streetlight orange
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.4f, 0.5f}),
        .fillColor = {0.15f, 0.18f, 0.3f},      // dashboard blue reflection
        .ambient = {0.08f, 0.07f, 0.09f},        // very dark — 2AM
        .pointPos = {0, 0, 0},
        .pointColor = {0, 0, 0},
        .pointRadius = 0,                         // no practical
    };
}

SceneLighting LightingPreset_Exterior(void) {
    // Paris at 2AM — moonlight blue-white, warm hotel entrance spill
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.8f, 0.2f}),
        .keyColor = {0.45f, 0.50f, 0.65f},       // moonlight — cool blue-white
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.3f, -0.8f}),
        .fillColor = {0.10f, 0.08f, 0.06f},      // ground bounce, subtle
        .ambient = {0.06f, 0.07f, 0.10f},         // deep night blue
        // Hotel entrance — warm spill from door
        .pointPos = {0, 2.0f, -8.0f},
        .pointColor = {0.9f, 0.7f, 0.4f},
        .pointRadius = 8.0f,
    };
}

SceneLighting LightingPreset_Lobby(void) {
    // Grand lobby — overhead chandeliers, warm marble reflection
    // Think: Wes Anderson hotel interior, symmetrical, golden
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.1f, -0.9f, -0.2f}),
        .keyColor = {1.1f, 0.95f, 0.7f},        // warm overhead chandelier
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.6f, 0.2f}),
        .fillColor = {0.25f, 0.22f, 0.18f},     // marble floor bounce
        .ambient = {0.18f, 0.17f, 0.15f},
        // Central chandelier
        .pointPos = {0, 3.5f, -5.0f},
        .pointColor = {1.0f, 0.85f, 0.55f},
        .pointRadius = 12.0f,
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
        // Hallway sconce
        .pointPos = {2.0f, 2.2f, -4.0f},
        .pointColor = {0.8f, 0.65f, 0.4f},
        .pointRadius = 6.0f,
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
        // Bedside lamp
        .pointPos = {3.0f, 1.5f, -3.0f},
        .pointColor = {0.9f, 0.7f, 0.45f},
        .pointRadius = 5.0f,
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
        // Mirror light bar
        .pointPos = {0, 2.3f, -2.5f},
        .pointColor = {0.9f, 0.92f, 0.95f},
        .pointRadius = 4.0f,
    };
}

SceneLighting LightingPreset_Balcony(void) {
    // Pre-dawn Paris — deep blue moonlight, warm city glow from below
    // The Eiffel Tower sparkles in the distance
    // Contemplative, expansive, melancholy beauty
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.4f, -0.6f, 0.3f}),
        .keyColor = {0.35f, 0.40f, 0.55f},      // moonlight — deep blue
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.8f, -0.5f}),
        .fillColor = {0.20f, 0.15f, 0.08f},     // city light bounce from below
        .ambient = {0.08f, 0.09f, 0.14f},        // deep night — mostly dark
        // Room light spilling out onto balcony
        .pointPos = {0, 2.0f, 3.0f},
        .pointColor = {0.7f, 0.55f, 0.35f},
        .pointRadius = 6.0f,
    };
}

SceneLighting LightingPreset_Space(void) {
    // Space hotel — cold starlight, Earth glow, minimal
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.5f, -0.5f}),
        .keyColor = {0.7f, 0.75f, 0.85f},       // starlight — cool white
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.10f, 0.18f, 0.30f},     // Earth glow — blue
        .ambient = {0.05f, 0.06f, 0.08f},        // deep space minimal
        .pointPos = {0, 0, 0},
        .pointColor = {0, 0, 0},
        .pointRadius = 0,
    };
}
