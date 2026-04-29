#ifndef EV_MODEL_REGISTRY_H
#define EV_MODEL_REGISTRY_H

#include "game_ctx.h"

#define MODEL_STARTUP_VAO_BUDGET 64

const ModelRegistryEntry *model_registry_entries(void);
int model_registry_count(void);
int model_registry_startup_vao_cost(void);
void init_model_registry_assets(void);
int preload_startup_model_assets(EVLighting *lighting);
bool ensure_model_asset_loaded(int index, EVLighting *lighting);

#endif
