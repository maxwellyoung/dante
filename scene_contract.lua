local SceneContract = {}

local DEFAULT_QA_EXPECTATIONS = {
    required_result = nil,
    max_deaths = nil,
    min_grapple_latches = nil,
    min_shots_hit = nil,
    min_shot_ricochets = nil,
    min_bank_kills = nil,
    min_accuracy = nil,
    min_distance = nil,
    min_dashes = nil,
    min_rolls = nil,
    min_crouch_time = nil,
    requires_encounter_clear = false,
    requires_removed_ability_notice = false,
    requires_environment_label = false,
}

local DEFAULT_SCENE = {
    id = "custom",
    title = "CUSTOM",
    subtitle = "",
    dramatic_question = "",
    transition_beat = "",
    objective_text = "",
    hud_objective_text = "",
    recovery_hint = "",
    mode = "custom",
    room_type = "custom",
    checkpoint_id = "start",
    encounter_id = nil,
    encounter_config = nil,
    beat_config = nil,
    music_state = "default",
    bg = { 0.08, 0.09, 0.11 },
    solid_color = { 0.28, 0.31, 0.36 },
    accent_color = { 0.92, 0.55, 0.18 },
    hazard_color = { 0.88, 0.22, 0.2 },
    sludge_color = { 0.32, 0.48, 0.24 },
    gate_color = { 0.92, 0.55, 0.18 },
    locked_gate_color = { 0.42, 0.22, 0.18 },
    fragments_required = 0,
    show_gate = true,
    removed_ability = "none",
    environment_hook = "",
    weapon_profile = {
        ricochets = 1,
        auto_aim = true,
    },
    abilities = {
        shoot = true,
        grapple = true,
        dash = true,
        crouch = true,
        roll = true,
    },
    wind_areas = {},
    qa_expectations = DEFAULT_QA_EXPECTATIONS,
}

local function copy_table(value)
    if type(value) ~= "table" then
        return value
    end

    local clone = {}
    for key, inner in pairs(value) do
        clone[key] = copy_table(inner)
    end
    return clone
end

local function merge_tables(base, override)
    local merged = copy_table(base)
    for key, value in pairs(override or {}) do
        if type(value) == "table" and type(merged[key]) == "table" then
            merged[key] = merge_tables(merged[key], value)
        else
            merged[key] = copy_table(value)
        end
    end
    return merged
end

function SceneContract.normalize(scene)
    local source = scene and scene.map and scene or { map = scene }
    local normalized = merge_tables(DEFAULT_SCENE, source or {})
    normalized.abilities = merge_tables(DEFAULT_SCENE.abilities, source and source.abilities or {})
    normalized.weapon_profile = merge_tables(DEFAULT_SCENE.weapon_profile, source and source.weapon_profile or {})
    normalized.qa_expectations = merge_tables(
        DEFAULT_QA_EXPECTATIONS,
        source and source.qa_expectations or {}
    )
    return normalized
end

return SceneContract
