#include "model_registry.h"

#include <stdio.h>
#include <string.h>

static const ModelRegistryEntry k_model_registry[] = {
    {"gibbons", "assets/gibbons.glb", MODEL_KIND_PROP, true, 17, true, MODEL_STATUS_ACTIVE},
    {"taxi_driver", "assets/taxi_driver.glb", MODEL_KIND_PROP, true, 6, true, MODEL_STATUS_ACTIVE},
    {"cart", "assets/cart.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"champagne_glasses", "assets/champagne_glasses.glb", MODEL_KIND_PROP, true, 4, true, MODEL_STATUS_ACTIVE},
    {"telephone", "assets/telephone.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"suitcase", "assets/suitcase.glb", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE},
    {"telescope", "assets/telescope.glb", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE},
    {"bed", "assets/bed.glb", MODEL_KIND_PROP, true, 5, true, MODEL_STATUS_ACTIVE},
    {"bathtub", "assets/bathtub.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"piano", "assets/piano.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"floor_lamp", "assets/floor_lamp.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"reception_desk", "assets/reception_desk.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"chandelier", "assets/chandelier.glb", MODEL_KIND_PROP, true, 3, true, MODEL_STATUS_ACTIVE},
    {"potted_plant", "assets/potted_plant.glb", MODEL_KIND_PROP, true, 5, true, MODEL_STATUS_ACTIVE},
    {"armchair", "assets/armchair.glb", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE},
    {"sofa", "assets/sofa.glb", MODEL_KIND_PROP, false, 2, true, MODEL_STATUS_ACTIVE},
    {"wine_glass", "assets/wine_glass.glb", MODEL_KIND_PROP, false, 2, true, MODEL_STATUS_ACTIVE},
    {"photograph_frame", "assets/photograph_frame.glb", MODEL_KIND_PROP, false, 2, true, MODEL_STATUS_ACTIVE},
    {"standing_ashtray", "assets/standing_ashtray.glb", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE},
    {"record_player", "assets/record_player.glb", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE},
    {"room_service_tray", "assets/room_service_tray.glb", MODEL_KIND_PROP, false, 2, true, MODEL_STATUS_ACTIVE},
    {"desk_lamp", "assets/desk_lamp.glb", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE},
    {"ice_bucket", "assets/ice_bucket.glb", MODEL_KIND_PROP, false, 4, true, MODEL_STATUS_ACTIVE},
    {"elevator_car", "assets/elevator_car.glb", MODEL_KIND_SHELL, false, 5, true, MODEL_STATUS_ACTIVE},
};

static int load_model_asset_slot(int index, EVLighting *lighting) {
    if (index < 0 || index >= g.model_asset_count) return 0;

    ModelAsset *ma = &g.model_assets[index];
    if (ma->loaded) return 1;

    if (ma->status != MODEL_STATUS_ACTIVE) {
        fprintf(stderr, "[EV] WARNING: model asset '%s' is dormant\n", ma->name);
        return 0;
    }
    if (!ma->path || !FileExists(ma->path)) {
        fprintf(stderr, "[EV] WARNING: model asset '%s' missing at '%s'\n",
                ma->name, ma->path ? ma->path : "(null)");
        return 0;
    }

    ma->model = LoadModel(ma->path);
    if (ma->model.meshCount <= 0) {
        UnloadModel(ma->model);
        ma->model = (Model){0};
        fprintf(stderr, "[EV] WARNING: model asset '%s' failed to load\n", ma->name);
        return 0;
    }

    if (lighting && lighting->ready) {
        for (int mi = 0; mi < ma->model.materialCount; mi++)
            ma->model.materials[mi].shader = lighting->shader;
    }

    if (IsFileExtension(ma->path, ".glb")) {
        long fsize = GetFileLength(ma->path);
        if (fsize > 0 && fsize < 5 * 1024 * 1024)
            ma->anims = LoadModelAnimations(ma->path, &ma->anim_count);
    }

    ma->loaded = true;
    printf("[EV] Model asset '%s' loaded — %d meshes, %d mats, %d anims [%s]\n",
           ma->name, ma->model.meshCount, ma->model.materialCount, ma->anim_count,
           ma->startup_load ? "startup" : "lazy");
    if (strcmp(ma->name, "gibbons") == 0) {
        BoundingBox bb = GetModelBoundingBox(ma->model);
        printf("[GIBBONS DIAG] BB min=(%.3f,%.3f,%.3f) max=(%.3f,%.3f,%.3f) size=(%.3f,%.3f,%.3f)\n",
               bb.min.x, bb.min.y, bb.min.z, bb.max.x, bb.max.y, bb.max.z,
               bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z);
    }
    fflush(stdout);
    return 1;
}

const ModelRegistryEntry *model_registry_entries(void) {
    return k_model_registry;
}

int model_registry_count(void) {
    return (int)(sizeof(k_model_registry) / sizeof(k_model_registry[0]));
}

int model_registry_startup_vao_cost(void) {
    int total = 0;
    for (int i = 0; i < model_registry_count(); i++) {
        const ModelRegistryEntry *entry = &k_model_registry[i];
        if (entry->status == MODEL_STATUS_ACTIVE && entry->startup_load)
            total += entry->estimated_vao_cost;
    }
    return total;
}

void init_model_registry_assets(void) {
    int count = model_registry_count();
    if (count > MAX_MODEL_ASSETS) {
        fprintf(stderr, "[EV] ERROR: model registry overflow (%d/%d)\n", count, MAX_MODEL_ASSETS);
        count = MAX_MODEL_ASSETS;
    }

    memset(g.model_assets, 0, sizeof(g.model_assets));
    g.model_asset_count = count;

    for (int i = 0; i < count; i++) {
        const ModelRegistryEntry *entry = &k_model_registry[i];
        ModelAsset *ma = &g.model_assets[i];
        strncpy(ma->name, entry->name, sizeof(ma->name) - 1);
        ma->name[sizeof(ma->name) - 1] = '\0';
        ma->path = entry->path;
        ma->kind = entry->kind;
        ma->startup_load = entry->startup_load;
        ma->estimated_vao_cost = entry->estimated_vao_cost;
        ma->preserve_blender_colors = entry->preserve_blender_colors;
        ma->status = entry->status;
    }
}

int preload_startup_model_assets(EVLighting *lighting) {
    int loaded_count = 0;
    int startup_cost = model_registry_startup_vao_cost();

    printf("[EV] Model registry: %d entries, startup VAO budget %d/%d\n",
           g.model_asset_count, startup_cost, MODEL_STARTUP_VAO_BUDGET);

    for (int i = 0; i < g.model_asset_count; i++) {
        ModelAsset *ma = &g.model_assets[i];
        if (ma->status != MODEL_STATUS_ACTIVE || !ma->startup_load)
            continue;
        if (load_model_asset_slot(i, lighting))
            loaded_count++;
    }

    printf("[EV] Loaded %d startup model asset(s)\n", loaded_count);
    return loaded_count;
}

bool ensure_model_asset_loaded(int index, EVLighting *lighting) {
    return load_model_asset_slot(index, lighting) != 0;
}
