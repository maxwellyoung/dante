// lighting.c — Custom shader lighting for EV engine
// Two-light setup: warm key + cool fill, rim light, color temperature split

#include "lighting.h"
#include "raymath.h"
#include <stdio.h>

// ============================================================
// VERTEX SHADER
// ============================================================
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

// ============================================================
// FRAGMENT SHADER — warm key + cool fill + rim + color temp fog
// ============================================================
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
    "\n"
    "    vec3 norm = normalize(fragNormal);\n"
    "\n"
    "    // Key light — half-Lambert with stronger contrast\n"
    "    float NdotL = dot(norm, -lightDir);\n"
    "    float halfLambert = NdotL * 0.5 + 0.5;\n"
    "    halfLambert = halfLambert * halfLambert;\n"
    "    vec3 keyDiffuse = lightColor * halfLambert * 1.3;\n"
    "\n"
    "    // Fill light — cool blue from below-behind, softer\n"
    "    float NdotF = dot(norm, -fillDir);\n"
    "    float fillLambert = max(NdotF * 0.5 + 0.5, 0.0);\n"
    "    vec3 fillDiffuse = fillColor * fillLambert * 0.4;\n"
    "\n"
    "    // Rim light — stronger for visible edge definition\n"
    "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
    "    float rim = 1.0 - max(dot(viewDir, norm), 0.0);\n"
    "    rim = pow(rim, 2.5) * 0.25;\n"
    "    vec3 rimColor = mix(fillColor, lightColor, 0.5) * rim;\n"
    "\n"
    "    // Color temperature split: warm on lit faces, cool ambient on shadow\n"
    "    vec3 warmAmbient = ambient * vec3(1.1, 1.0, 0.85);\n"
    "    vec3 coolAmbient = ambient * vec3(0.85, 0.95, 1.15);\n"
    "    float shadow = smoothstep(-0.2, 0.3, NdotL);\n"
    "    vec3 ambientMix = mix(coolAmbient, warmAmbient, shadow);\n"
    "\n"
    "    // Combine\n"
    "    vec3 lit = baseColor * (ambientMix + keyDiffuse + fillDiffuse) + rimColor;\n"
    "\n"
    "    // Distance fog\n"
    "    float dist = length(viewPos - fragPosition);\n"
    "    float fog = 1.0 - exp(-fogDensity * dist * dist);\n"
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

        int matNormalLoc = GetShaderLocation(lighting.shader, "matNormal");
        lighting.shader.locs[SHADER_LOC_MATRIX_NORMAL] = matNormalLoc;

        // Key light — warm from above-front, higher intensity
        Vector3 lightDir = Vector3Normalize((Vector3){-0.3f, -0.8f, -0.4f});
        float lightDirArr[3] = {lightDir.x, lightDir.y, lightDir.z};
        SetShaderValue(lighting.shader, lighting.lightDirLoc, lightDirArr, SHADER_UNIFORM_VEC3);

        float lightColor[3] = {1.1f, 0.95f, 0.75f};  // warm, punchy
        SetShaderValue(lighting.shader, lighting.lightColorLoc, lightColor, SHADER_UNIFORM_VEC3);

        // Fill light — cool blue from below-behind
        Vector3 fillDir = Vector3Normalize((Vector3){0.4f, 0.6f, 0.3f});
        float fillDirArr[3] = {fillDir.x, fillDir.y, fillDir.z};
        SetShaderValue(lighting.shader, lighting.fillDirLoc, fillDirArr, SHADER_UNIFORM_VEC3);

        float fillColor[3] = {0.4f, 0.5f, 0.7f};  // cool blue
        SetShaderValue(lighting.shader, lighting.fillColorLoc, fillColor, SHADER_UNIFORM_VEC3);

        // Ambient — darker for more contrast
        float ambient[3] = {0.15f, 0.12f, 0.10f};
        SetShaderValue(lighting.shader, lighting.ambientLoc, ambient, SHADER_UNIFORM_VEC3);

        lighting.ready = true;
        printf("[EV] Custom lighting shader loaded (2-light setup)\n");
    } else {
        printf("[EV] WARNING: Failed to load lighting shader\n");
        lighting.ready = false;
    }

    return lighting;
}

void UnloadEVLighting(EVLighting *lighting) {
    if (lighting->ready) {
        UnloadShader(lighting->shader);
        lighting->ready = false;
    }
}

void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity) {
    if (!lighting->ready) return;

    float viewPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lighting->shader, lighting->viewPosLoc, viewPos, SHADER_UNIFORM_VEC3);

    float fogCol[3] = {fogColor.r / 255.0f, fogColor.g / 255.0f, fogColor.b / 255.0f};
    SetShaderValue(lighting->shader, lighting->fogColorLoc, fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);
}
