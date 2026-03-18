// scene_registry.c — Function pointer table indexed by GameState
#include "game_ctx.h"

extern GameCtx g;

// Forward declarations — implemented in scene_*.c files (or main.c during migration)
void title_load(void);
void title_update(float dt);
void taxi_load(void);
void taxi_update(float dt);
void driving_update(float dt);
void exterior_load(void);
void exterior_update(float dt);
void lobby_load(void);
void lobby_update(float dt);
void elevator_load(void);
void elevator_update(float dt);
void hallway_load(void);
void hallway_update(float dt);
void room_load(void);
void room_update(float dt);
void bathroom_load(void);
void bathroom_update(float dt);
void balcony_load(void);
void balcony_update(float dt);
void bed_load(void);
void bed_update(float dt);
void stars_load(void);
void stars_update(float dt);
void hyperspace_load(void);
void hyperspace_update(float dt);
void space_lobby_load(void);
void space_lobby_update(float dt);
void corridor_load(void);
void corridor_update(float dt);
void suite_load(void);
void suite_update(float dt);
void paris_dream_load(void);
void paris_dream_update(float dt);
void cleaned_suite_load(void);
void cleaned_suite_update(float dt);
void montage_load(void);
void montage_update(float dt);
void return_taxi_load(void);
void return_taxi_update(float dt);

const SceneDesc scene_descs[] = {
    [STATE_TITLE]          = { .load = title_load,       .update = title_update,       .indoor = true  },
    [STATE_CAR]            = { .load = taxi_load,        .update = taxi_update,         .indoor = true  },
    [STATE_DRIVING]        = { .load = NULL,             .update = driving_update,      .indoor = true  },
    [STATE_HOTEL_EXT]      = { .load = exterior_load,    .update = exterior_update,     .indoor = false },
    [STATE_LOBBY]          = { .load = lobby_load,       .update = lobby_update,        .indoor = true  },
    [STATE_ELEVATOR]       = { .load = elevator_load,    .update = elevator_update,     .indoor = true  },
    [STATE_HALLWAY]        = { .load = hallway_load,     .update = hallway_update,      .indoor = true  },
    [STATE_ROOM]           = { .load = room_load,        .update = room_update,         .indoor = true  },
    [STATE_BATHROOM]       = { .load = bathroom_load,    .update = bathroom_update,     .indoor = true  },
    [STATE_BALCONY]        = { .load = balcony_load,     .update = balcony_update,      .indoor = false },
    [STATE_BED]            = { .load = bed_load,         .update = bed_update,          .indoor = true  },
    [STATE_STARS]          = { .load = stars_load,       .update = stars_update,        .indoor = true  },
    [STATE_HYPERSPACE]     = { .load = hyperspace_load,  .update = hyperspace_update,   .indoor = false },
    [STATE_SPACE_LOBBY]    = { .load = space_lobby_load, .update = space_lobby_update,  .indoor = false },
    [STATE_SPACE_CORRIDOR] = { .load = corridor_load,    .update = corridor_update,     .indoor = true  },
    [STATE_SPACE_SUITE]    = { .load = suite_load,       .update = suite_update,        .indoor = true  },
    [STATE_PARIS_DREAM]    = { .load = paris_dream_load, .update = paris_dream_update,  .indoor = true  },
    [STATE_CLEANED_SUITE]  = { .load = cleaned_suite_load, .update = cleaned_suite_update, .indoor = true },
    [STATE_MONTAGE]        = { .load = montage_load,       .update = montage_update,      .indoor = true },
    [STATE_RETURN_TAXI]    = { .load = return_taxi_load,  .update = return_taxi_update, .indoor = true  },
};

const int scene_desc_count = sizeof(scene_descs) / sizeof(scene_descs[0]);
