// skybox.h — Procedural skybox system for EV engine
// Auckland night. Deep void. Dawn. Paris haze. Hyperspace streaks.
// All GPU — no per-pixel CPU loops.
#ifndef EV_SKYBOX_H
#define EV_SKYBOX_H

#include "raylib.h"

typedef enum {
    SKY_NONE,           // interior — just ClearBackground
    SKY_AUCKLAND_NIGHT, // navy gradient, stars, crescent moon, light pollution
    SKY_SPACE_VOID,     // blue-purple gradient, parallax star layers, nebula wisps
    SKY_DAWN,           // blue→pink→gold, cloud bands, warm horizon
    SKY_PARIS_DREAM,    // golden volumetric haze, animated fog
    SKY_HYPERSPACE,     // radial streaks from center
} SkyType;

typedef struct {
    Shader shader;
    int timeLoc;
    int skyTypeLoc;
    int resolutionLoc;
    int viewMatrixLoc;
    int param1Loc;      // per-type parameter (ascent fade, streak speed, etc.)
    int param2Loc;      // reserved
    bool ready;
} EVSkybox;

EVSkybox LoadEVSkybox(void);
void UnloadEVSkybox(EVSkybox *sky);
void DrawSkybox(EVSkybox *sky, Camera3D camera, SkyType type, float time, float param1, float param2);

#endif
