// lighting.h — Custom shader-based lighting for the EV engine
// Directional key + fill + point light + ambient + fog
// Per-scene presets: taxi, exterior, lobby, hallway, room, balcony

#ifndef EV_LIGHTING_H
#define EV_LIGHTING_H

#include "raylib.h"
#include "raymath.h"

#define MAX_POINT_LIGHTS 4
#define SHADOW_MAP_SIZE 2048

// Per-scene lighting configuration — set once per scene transition
typedef struct {
    Vector3 keyDir;         // key light direction (normalized)
    float keyColor[3];      // key light color (HDR, can exceed 1.0)
    Vector3 fillDir;        // fill light direction (normalized)
    float fillColor[3];     // fill light color
    float ambient[3];       // ambient light color
    // Point lights (practicals: lamps, candles, fixtures)
    Vector3 pointPos[MAX_POINT_LIGHTS];
    float pointColor[MAX_POINT_LIGHTS][3];
    float pointRadius[MAX_POINT_LIGHTS];   // 0 = disabled
    // Environment reflection — per-scene color for metallic/glass surfaces
    float reflectionColor[3];  // {0,0,0} = no reflection
    // Indirect light probes — cheap bounce approximation
    Vector3 bouncePos[3];      // {0,0,0} = disabled
    float bounceColor[3][3];   // RGB per probe
} SceneLighting;

typedef struct {
    Shader shader;
    int viewPosLoc;
    int ambientLoc;
    int fogColorLoc;
    int fogDensityLoc;
    int lightDirLoc;
    int lightColorLoc;
    int fillDirLoc;
    int fillColorLoc;
    // Point lights (4)
    int pointPosLoc[MAX_POINT_LIGHTS];
    int pointColorLoc[MAX_POINT_LIGHTS];
    int pointRadiusLoc[MAX_POINT_LIGHTS];
    // Material
    int materialIdLoc;
    // Environment reflection + bounce probes
    int reflectionColorLoc;
    int bouncePosLoc[3];
    int bounceColorLoc[3];
    // Shadow mapping
    unsigned int shadowFBO;
    unsigned int shadowDepthTex;
    Shader shadowShader;
    int shadowMapLoc;            // sampler2D in lighting shader
    int lightSpaceMatrixLoc;     // mat4 in lighting shader
    int shadowMvpLoc;            // mvp in shadow shader
    Matrix lightSpaceMatrix;
    bool shadowReady;
    bool shadowPassRan;      // true if depth was written this frame
    SceneLighting activePreset;  // last SetSceneLighting — shadow pass reads keyDir
    bool ready;
} EVLighting;

EVLighting LoadEVLighting(void);
void UnloadEVLighting(EVLighting *lighting);
void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity);

// Shadow mapping
void CreateShadowMap(EVLighting *lighting);
void DestroyShadowMap(EVLighting *lighting);
void UpdateShadowMatrix(EVLighting *lighting, Vector3 keyDir, Vector3 sceneCenter, float sceneRadius);
void BindShadowMap(EVLighting *lighting);
void UnbindShadowMap(void);

// Apply a per-scene lighting preset
void SetSceneLighting(EVLighting *lighting, SceneLighting preset);

// Move the point light dynamically (e.g. lamp turned on, candle lit)
void SetPointLight(EVLighting *lighting, float x, float y, float z,
                   float r, float g, float b, float radius);

void SetPointLightIdx(EVLighting *lighting, int index, float x, float y, float z,
                      float r, float g, float b, float radius);

// Set per-wall material ID (called before each DrawModelEx)
void SetMaterialId(EVLighting *lighting, int materialId);

// Built-in presets per scene
SceneLighting LightingPreset_Taxi(void);
SceneLighting LightingPreset_Exterior(void);
SceneLighting LightingPreset_Lobby(void);
SceneLighting LightingPreset_Elevator(void);
SceneLighting LightingPreset_ElevatorSpace(void);
SceneLighting LightingPreset_Hallway(void);
SceneLighting LightingPreset_Room(void);
SceneLighting LightingPreset_Bathroom(void);
SceneLighting LightingPreset_Balcony(void);
SceneLighting LightingPreset_SpaceLobby(void);
SceneLighting LightingPreset_SpaceHotel(void);
SceneLighting LightingPreset_SpaceCorridor(void);
SceneLighting LightingPreset_SpaceSuite(void);
SceneLighting LightingPreset_Hyperspace(void);
SceneLighting LightingPreset_ParisDream(void);
SceneLighting LightingPreset_ReturnTaxi(void);

#endif
