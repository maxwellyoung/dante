#include <stdio.h>
#include <assert.h>
#ifndef HEADLESS_TEST
#define HEADLESS_TEST
#endif
typedef struct { float x, y, z; } Vector3;
typedef struct { float x, y, z, w; } Vector4;
typedef struct { unsigned char r, g, b, a; } Color;
typedef struct { Vector3 position, target, up; float fovy; int projection; } Camera3D;
typedef struct { unsigned int id; int *locs; } Shader;
typedef struct { int dummy; } Sound;
typedef struct { int dummy; } Music;
typedef struct { int dummy; } Model;
typedef struct { int dummy; } RenderTexture2D;
typedef struct { float m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15; } Matrix;
typedef struct { unsigned int id; } Texture2D;
typedef Vector4 Quaternion;
#define RAYLIB_H
#define RAYMATH_H
#include "../src/ev_types.h"
#include "../src/scene.h"
#include "../src/player.h"
#include "../src/audio.h"
#include "../src/render.h"
#include "../src/lighting.h"
#define GAMESTATE_COUNT (STATE_RETURN_TAXI + 1)
int main(void) {
    printf("EV Engine — Headless Test Suite\n");
    printf("================================\n\n");
    printf("Checking constants...\n");
    assert(MAX_WALLS == 2048);
    assert(MAX_OBJECTS == 64);
    assert(RENDER_W == 960);
    assert(RENDER_H == 600);
    printf("  MAX_WALLS=%d MAX_OBJECTS=%d RENDER=%dx%d  OK\n", MAX_WALLS, MAX_OBJECTS, RENDER_W, RENDER_H);
    printf("\nChecking struct sizes...\n");
    assert(sizeof(Wall) > 0);
    assert(sizeof(Player) > 0);
    assert(sizeof(Scene) > 0);
    assert(sizeof(CinematicFX) > 0);
    printf("  Wall=%zu Player=%zu Scene=%zu CinematicFX=%zu  OK\n",
           sizeof(Wall), sizeof(Player), sizeof(Scene), sizeof(CinematicFX));
    printf("\nChecking GameState enum...\n");
    assert(GAMESTATE_COUNT == 20);
    assert(STATE_SPLASH == 0);
    assert(STATE_TITLE == 1);
    printf("  GameState count=%d SPLASH=%d TITLE=%d RETURN_TAXI=%d  OK\n",
           GAMESTATE_COUNT, STATE_SPLASH, STATE_TITLE, STATE_RETURN_TAXI);
    printf("\nChecking SurfaceType...\n");
    assert(SURFACE_MARBLE == 0);
    assert(SURFACE_CARPET == 1);
    assert(SURFACE_WOOD == 2);
    printf("  SurfaceType  OK\n");
    printf("\n================================\n");
    printf("All checks passed.\n");
    return 0;
}
