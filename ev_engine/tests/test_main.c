// test_main.c — Headless smoke test for EV engine types and constants
// Does NOT link against Raylib — only checks struct sizes, constants, enum counts.

#include <stdio.h>
#include <assert.h>

// Raylib type stubs for headless compilation
#ifndef HEADLESS_TEST
#define HEADLESS_TEST
#endif

// Minimal Raylib type definitions so headers can be included without Raylib
typedef struct { float x, y, z; } Vector3;
typedef struct { float x, y, z, w; } Vector4;
typedef struct { unsigned char r, g, b, a; } Color;
typedef struct {
    Vector3 position;
    Vector3 target;
    Vector3 up;
    float fovy;
    int projection;
} Camera3D;
typedef struct { unsigned int id; int *locs; } Shader;
typedef struct { int dummy; } Sound;
typedef struct { int dummy; } Model;
typedef struct { int dummy; } RenderTexture2D;

// Provide the raylib.h guard so headers don't try to include it
#define RAYLIB_H

#include "../src/ev_types.h"
#include "../src/scene.h"
#include "../src/player.h"
#include "../src/audio.h"
#include "../src/render.h"
#include "../src/lighting.h"

// Count enum values by using the last known value + 1
#define GAMESTATE_COUNT (STATE_STARS + 1)

int main(void) {
    printf("EV Engine — Headless Test Suite\n");
    printf("================================\n\n");

    // Constants
    printf("Checking constants...\n");
    assert(MAX_WALLS == 768);
    assert(MAX_OBJECTS == 64);
    assert(RENDER_W == 480);
    assert(RENDER_H == 300);
    printf("  MAX_WALLS   = %d  OK\n", MAX_WALLS);
    printf("  MAX_OBJECTS = %d  OK\n", MAX_OBJECTS);
    printf("  RENDER_W    = %d  OK\n", RENDER_W);
    printf("  RENDER_H    = %d  OK\n", RENDER_H);

    // Struct sizes (must be > 0, sanity check)
    printf("\nChecking struct sizes...\n");
    assert(sizeof(Wall) > 0);
    assert(sizeof(Player) > 0);
    assert(sizeof(Scene) > 0);
    assert(sizeof(InteractObject) > 0);
    assert(sizeof(EVAudio) > 0);
    assert(sizeof(EVPostFX) > 0);
    assert(sizeof(EVLighting) > 0);
    printf("  sizeof(Wall)           = %zu  OK\n", sizeof(Wall));
    printf("  sizeof(Player)         = %zu  OK\n", sizeof(Player));
    printf("  sizeof(Scene)          = %zu  OK\n", sizeof(Scene));
    printf("  sizeof(InteractObject) = %zu  OK\n", sizeof(InteractObject));
    printf("  sizeof(EVAudio)        = %zu  OK\n", sizeof(EVAudio));
    printf("  sizeof(EVPostFX)       = %zu  OK\n", sizeof(EVPostFX));
    printf("  sizeof(EVLighting)     = %zu  OK\n", sizeof(EVLighting));

    // GameState enum count — should be 12
    printf("\nChecking GameState enum...\n");
    assert(GAMESTATE_COUNT == 12);
    printf("  GameState count = %d  OK\n", GAMESTATE_COUNT);
    printf("  STATE_TITLE     = %d\n", STATE_TITLE);
    printf("  STATE_BATHROOM  = %d\n", STATE_BATHROOM);
    printf("  STATE_STAIRWELL = %d\n", STATE_STAIRWELL);
    printf("  STATE_STARS     = %d\n", STATE_STARS);

    // Surface types
    printf("\nChecking SurfaceType enum...\n");
    assert(SURFACE_MARBLE == 0);
    assert(SURFACE_CARPET == 1);
    assert(SURFACE_WOOD == 2);
    printf("  SurfaceType values OK\n");

    printf("\n================================\n");
    printf("All checks passed.\n");
    return 0;
}
