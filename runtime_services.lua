local RuntimeServices = {}
RuntimeServices.__index = RuntimeServices

function RuntimeServices:new()
    local class = self or RuntimeServices
    return setmetatable({
        level = nil,
        player = nil,
        camera = nil,
        sfx = nil,
        effects = nil,
        ui = nil,
        background = nil,
        collectibles = nil,
        enemies = nil,
        projectiles = nil,
        autoplay = nil,
        encounters = nil,
        run_stats = nil,
        current_scene = nil,
        game_mode = "proving_ground",
        room_index = 1,
        room_count = 1,
        debug_overlay = false,
        trigger_hitstop = nil,
        mark_meaningful_action = nil,
        record_removed_ability_notice = nil,
        record_environment_label = nil,
        record_encounter_complete = nil,
        record_checkpoint_activated = nil,
        record_gate_entered = nil,
        advance_current_room = nil,
        quick_reset = nil,
        respawn_player = nil,
        select_flow_room = nil,
        input_override = nil,
    }, class)
end

function RuntimeServices:set_scene(scene, level)
    self.current_scene = scene
    self.level = level
end

function RuntimeServices:set_progress(game_mode, room_index, room_count)
    self.game_mode = game_mode or self.game_mode
    self.room_index = room_index or self.room_index
    self.room_count = room_count or self.room_count
end

return RuntimeServices
