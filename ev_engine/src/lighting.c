// lighting.c — Custom shader lighting for EV engine
// Dramatic contrast. Warm key, cool fill. Tadao Ando, not horror.

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
    "    // Fill light — warm from below, soft\n"
    "    float NdotF = dot(norm, -fillDir);\n"
    "    float fillLambert = max(NdotF * 0.5 + 0.5, 0.0);\n"
    "    vec3 fillDiffuse = fillColor * fillLambert * 0.5;\n"
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
    "    vec3 lit = baseColor * (ambient * ao * (1.0 - selfLit) + keyDiffuse + fillDiffuse + selfLit) + rimColor + specColor + vec3(fresnel);\n"
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
        int matNormalLoc = GetShaderLocation(lighting.shader, "matNormal");
        lighting.shader.locs[SHADER_LOC_MATRIX_NORMAL] = matNormalLoc;

        // Key light — golden from above-front
        Vector3 lightDir = Vector3Normalize((Vector3){-0.2f, -0.7f, -0.3f});
        float lightDirArr[3] = {lightDir.x, lightDir.y, lightDir.z};
        SetShaderValue(lighting.shader, lighting.lightDirLoc, lightDirArr, SHADER_UNIFORM_VEC3);
        float lightColor[3] = {1.2f, 1.05f, 0.82f};
        SetShaderValue(lighting.shader, lighting.lightColorLoc, lightColor, SHADER_UNIFORM_VEC3);

        // Fill — warm from below-side
        Vector3 fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f});
        float fillDirArr[3] = {fillDir.x, fillDir.y, fillDir.z};
        SetShaderValue(lighting.shader, lighting.fillDirLoc, fillDirArr, SHADER_UNIFORM_VEC3);
        float fillColor[3] = {0.30f, 0.35f, 0.45f};  // cooler blue fill — more contrast
        SetShaderValue(lighting.shader, lighting.fillColorLoc, fillColor, SHADER_UNIFORM_VEC3);

        // Ambient — lower for dramatic Ando shadows
        float ambient[3] = {0.22f, 0.21f, 0.20f};
        SetShaderValue(lighting.shader, lighting.ambientLoc, ambient, SHADER_UNIFORM_VEC3);

        lighting.ready = true;
        printf("[EV] Lighting loaded — bright, warm, golden\n");
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
