// lighting.h — Custom shader-based lighting for the EV engine
#ifndef EV_LIGHTING_H
#define EV_LIGHTING_H
#include "raylib.h"
#include "raymath.h"
#define MAX_POINT_LIGHTS 8
#define SHADOW_MAP_SIZE 2048
typedef struct {
    Vector3 keyDir; float keyColor[3];
    Vector3 fillDir; float fillColor[3];
    float ambient[3];
    Vector3 pointPos[MAX_POINT_LIGHTS];
    float pointColor[MAX_POINT_LIGHTS][3];
    float pointRadius[MAX_POINT_LIGHTS];
    float pointFlicker[MAX_POINT_LIGHTS];
    float pointPulse[MAX_POINT_LIGHTS];
    float pointPhase[MAX_POINT_LIGHTS];
    float reflectionColor[3];
    Vector3 bouncePos[3];
    float bounceColor[3][3];
} SceneLighting;
typedef struct {
    Shader shader;
    int viewPosLoc, ambientLoc, fogColorLoc, fogDensityLoc;
    int lightDirLoc, lightColorLoc, fillDirLoc, fillColorLoc;
    int pointPosLoc[MAX_POINT_LIGHTS];
    int pointColorLoc[MAX_POINT_LIGHTS];
    int pointRadiusLoc[MAX_POINT_LIGHTS];
    int materialIdLoc, reflectionColorLoc;
    int bouncePosLoc[3], bounceColorLoc[3];
    unsigned int shadowFBO, shadowDepthTex;
    Shader shadowShader;
    int shadowMapLoc, lightSpaceMatrixLoc, shadowMvpLoc;
    Matrix lightSpaceMatrix;
    bool shadowReady, shadowPassRan;
    SceneLighting activePreset;
    bool ready;
} EVLighting;
EVLighting LoadEVLighting(void);
void UnloadEVLighting(EVLighting *lighting);
void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity);
void AnimateLights(EVLighting *lighting, float time);
void SetLightingWarmth(EVLighting *lighting, float warmth);
void CreateShadowMap(EVLighting *lighting);
void DestroyShadowMap(EVLighting *lighting);
void UpdateShadowMatrix(EVLighting *lighting, Vector3 keyDir, Vector3 sceneCenter, float sceneRadius);
void BindShadowMap(EVLighting *lighting);
void UnbindShadowMap(void);
void SetSceneLighting(EVLighting *lighting, SceneLighting preset);
void SetPointLight(EVLighting *lighting, float x, float y, float z, float r, float g, float b, float radius);
void SetPointLightIdx(EVLighting *lighting, int index, float x, float y, float z, float r, float g, float b, float radius);
void SetMaterialId(EVLighting *lighting, int materialId);
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
