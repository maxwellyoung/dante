// lighting.h — Custom shader-based lighting for the EV engine
// Directional key + fill + point light + ambient + fog
// Per-scene presets: taxi, exterior, lobby, hallway, room, balcony

#ifndef EV_LIGHTING_H
#define EV_LIGHTING_H

#include "raylib.h"
#include "raymath.h"

// Per-scene lighting configuration — set once per scene transition
typedef struct {
    Vector3 keyDir;         // key light direction (normalized)
    float keyColor[3];      // key light color (HDR, can exceed 1.0)
    Vector3 fillDir;        // fill light direction (normalized)
    float fillColor[3];     // fill light color
    float ambient[3];       // ambient light color
    // Point light (practical: lamp, candle, fixture)
    Vector3 pointPos;       // world position
    float pointColor[3];    // color
    float pointRadius;      // falloff radius (0 = disabled)
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
    // Point light
    int pointPosLoc;
    int pointColorLoc;
    int pointRadiusLoc;
    // Material
    int materialIdLoc;
    bool ready;
} EVLighting;

EVLighting LoadEVLighting(void);
void UnloadEVLighting(EVLighting *lighting);
void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity);

// Apply a per-scene lighting preset
void SetSceneLighting(EVLighting *lighting, SceneLighting preset);

// Move the point light dynamically (e.g. lamp turned on, candle lit)
void SetPointLight(EVLighting *lighting, float x, float y, float z,
                   float r, float g, float b, float radius);

// Set per-wall material ID (called before each DrawModelEx)
void SetMaterialId(EVLighting *lighting, int materialId);

// Built-in presets per scene
SceneLighting LightingPreset_Taxi(void);
SceneLighting LightingPreset_Exterior(void);
SceneLighting LightingPreset_Lobby(void);
SceneLighting LightingPreset_Elevator(void);
SceneLighting LightingPreset_Hallway(void);
SceneLighting LightingPreset_Room(void);
SceneLighting LightingPreset_Bathroom(void);
SceneLighting LightingPreset_Balcony(void);
SceneLighting LightingPreset_SpaceLobby(void);
SceneLighting LightingPreset_SpaceCorridor(void);
SceneLighting LightingPreset_SpaceSuite(void);

#endif
