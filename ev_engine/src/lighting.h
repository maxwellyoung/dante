// lighting.h — Custom shader-based lighting for the EV engine
// Directional light + fill light + ambient + per-face normal shading + fog

#ifndef EV_LIGHTING_H
#define EV_LIGHTING_H

#include "raylib.h"

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
    bool ready;
} EVLighting;

EVLighting LoadEVLighting(void);
void UnloadEVLighting(EVLighting *lighting);
void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity);

#endif
