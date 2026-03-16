// lighting.c — Custom shader lighting for EV engine
// Embedded GLSL shaders — no external files needed

#include "lighting.h"
#include "raymath.h"
#include <stdio.h>

// ============================================================
// VERTEX SHADER — passes world position and normal to fragment
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
// FRAGMENT SHADER — directional light + ambient + distance fog
// Jeremy Blake aesthetic: warm light, saturated fog, soft normals
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
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 texColor = texture(texture0, fragTexCoord);\n"
    "    vec3 baseColor = texColor.rgb * colDiffuse.rgb * fragColor.rgb;\n"
    "\n"
    "    // Directional light with half-lambert for softer shading\n"
    "    vec3 norm = normalize(fragNormal);\n"
    "    float NdotL = dot(norm, -lightDir);\n"
    "    float halfLambert = NdotL * 0.5 + 0.5;\n"
    "    halfLambert = halfLambert * halfLambert;  // softer falloff\n"
    "    vec3 diffuse = lightColor * halfLambert;\n"
    "\n"
    "    // Subtle rim light for depth\n"
    "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
    "    float rim = 1.0 - max(dot(viewDir, norm), 0.0);\n"
    "    rim = pow(rim, 3.0) * 0.15;\n"
    "\n"
    "    // Combine\n"
    "    vec3 lit = baseColor * (ambient + diffuse) + rim * lightColor * 0.3;\n"
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
        // Get uniform locations
        lighting.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lighting.shader, "matModel");
        lighting.shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(lighting.shader, "mvp");
        lighting.shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lighting.shader, "viewPos");

        lighting.viewPosLoc = GetShaderLocation(lighting.shader, "viewPos");
        lighting.ambientLoc = GetShaderLocation(lighting.shader, "ambient");
        lighting.fogColorLoc = GetShaderLocation(lighting.shader, "fogColor");
        lighting.fogDensityLoc = GetShaderLocation(lighting.shader, "fogDensity");
        lighting.lightDirLoc = GetShaderLocation(lighting.shader, "lightDir");
        lighting.lightColorLoc = GetShaderLocation(lighting.shader, "lightColor");

        // matNormal location
        int matNormalLoc = GetShaderLocation(lighting.shader, "matNormal");
        lighting.shader.locs[SHADER_LOC_MATRIX_NORMAL] = matNormalLoc;

        // Default values — warm interior light from above-front
        Vector3 lightDir = Vector3Normalize((Vector3){-0.3f, -0.8f, -0.4f});
        float lightDirArr[3] = {lightDir.x, lightDir.y, lightDir.z};
        SetShaderValue(lighting.shader, lighting.lightDirLoc, lightDirArr, SHADER_UNIFORM_VEC3);

        float lightColor[3] = {1.0f, 0.92f, 0.78f};  // warm white
        SetShaderValue(lighting.shader, lighting.lightColorLoc, lightColor, SHADER_UNIFORM_VEC3);

        float ambient[3] = {0.25f, 0.22f, 0.18f};  // warm ambient
        SetShaderValue(lighting.shader, lighting.ambientLoc, ambient, SHADER_UNIFORM_VEC3);

        lighting.ready = true;
        printf("[EV] Custom lighting shader loaded\n");
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
