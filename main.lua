g_player = nil
g_level = nil
g_camera = nil
g_sfx = nil
g_effects = nil
g_ui = nil
g_background = nil
g_collectibles = nil
g_enemies = nil
g_projectiles = nil
g_preview = nil
g_preview_mode = false
g_autoplay = nil
g_encounters = nil
g_input_override = {}
g_game_mode = "proving_ground"
g_current_circle = 0
g_current_scene = nil
g_current_flow = nil
g_current_room_index = 1
g_death_timer = 0
g_respawn_point = nil
g_debug_overlay = false
g_run_stats = nil
g_qa_capture = nil
g_services = nil
g_hitstop_timer = 0
g_trial_timer = 0
g_trial_active = false
g_trial_completed = false
g_hard_cut_timer = 0
g_hard_cut_pending = false
g_trial_score = nil
g_circle_score = 0
g_ambient_source = nil
g_ambient_circle_id = nil

g_native_width, g_native_height = 480, 270
g_main_canvas = nil

circle_data = require("circles")
local campaign = require("campaign")
local proving_ground = require("proving_ground")
local SceneContract = require("scene_contract")
local RuntimeServices = require("runtime_services")
Player = require("player")
Level = require("level")
Camera = require("camera")
Sfx = require("sfx")
Effects = require("effects")
UI = require("ui")
Background = require("background")
Collectibles = require("collectibles")
Enemies = require("enemies")
Projectiles = require("projectiles")
PreviewPlayer = require("preview_player")
EncounterController = require("encounter_controller")
local Autoplay = require("autoplay")

local native_is_down = love.keyboard.isDown

local function install_input_override()
    love.keyboard.isDown = function(...)
        local keys = { ... }
        for _, key in ipairs(keys) do
            if g_input_override[key] == true then
                return true
            end
        end
        return native_is_down(...)
    end
end

local function has_flag(args, flag)
    for _, value in ipairs(args or {}) do
        if value == flag then
            return true
        end
    end
    return false
end

local function get_flag_value(args, prefix)
    for _, value in ipairs(args or {}) do
        if value:sub(1, #prefix) == prefix then
            return value:sub(#prefix + 1)
        end
    end
    return nil
end

local function absolute_path(path)
    if not path or path == "" then
        return path
    end
    if path:sub(1, 1) == "/" then
        return path
    end
    return love.filesystem.getWorkingDirectory() .. "/" .. path
end

local function parent_directory(path)
    return path:match("^(.*)/[^/]+$") or "."
end

local function ensure_directory(path)
    if not path or path == "" then
        return
    end
    os.execute("mkdir -p " .. string.format("%q", path))
end

local function should_use_qa_window_mode(args)
    return has_flag(args, "--qa-background")
end

local function apply_game_window_mode(args)
    local options = { resizable = true, vsync = true }
    if should_use_qa_window_mode(args) then
        options = {
            resizable = false,
            vsync = false,
            borderless = true,
            centered = false,
            x = -2200,
            y = 40,
        }
    end

    love.window.setMode(g_native_width * 3, g_native_height * 3, options)
    love.window.setTitle("Infernal Ascent")

    if should_use_qa_window_mode(args) then
        love.mouse.setVisible(false)
        pcall(love.window.minimize)
    end
end

local function request_qa_capture_if_ready()
    if not g_qa_capture or g_qa_capture.requested or g_preview_mode then
        return
    end

    if (g_run_stats and g_run_stats.total_time or 0) < g_qa_capture.delay then
        return
    end

    g_qa_capture.requested = true
    ensure_directory(parent_directory(g_qa_capture.path))
    love.graphics.captureScreenshot(function(image_data)
        local ok, file_data = pcall(function()
            return image_data:encode("png")
        end)

        if ok and file_data then
            local handle = io.open(g_qa_capture.path, "wb")
            if handle then
                handle:write(file_data:getString())
                handle:close()
            end
        end

        g_qa_capture.completed = true
        os.exit(0)
    end)
end

local function init_run_stats()
    g_run_stats = {
        total_time = 0,
        room_time = 0,
        deaths = 0,
        room_deaths = 0,
        resets = 0,
        shots_fired = 0,
        shots_hit = 0,
        room_shots_fired = 0,
        room_shots_hit = 0,
        shot_ricochets = 0,
        room_shot_ricochets = 0,
        shot_bank_kills = 0,
        room_shot_bank_kills = 0,
        jumps = 0,
        room_jumps = 0,
        grapples_fired = 0,
        room_grapples_fired = 0,
        grapple_latches = 0,
        room_grapple_latches = 0,
        dashes = 0,
        room_dashes = 0,
        rolls = 0,
        room_rolls = 0,
        crouch_time = 0,
        room_crouch_time = 0,
        distance = 0,
        room_distance = 0,
        checkpoints = 0,
        room_checkpoints = 0,
        room_last_checkpoint_at = nil,
        room_last_checkpoint_id = nil,
        respawns = 0,
        room_respawns = 0,
        last_respawn_latency = nil,
        room_last_respawn_latency = nil,
        room_gate_entered = false,
        rooms_cleared = 0,
        first_meaningful_action_at = nil,
        room_first_meaningful_action_at = nil,
        room_environment_label_seen = false,
        room_environment_label = nil,
        room_removed_ability_notice_shown = false,
        room_encounter_complete = false,
        room_notice_events = 0,
    }
    if g_services then
        g_services.run_stats = g_run_stats
    end
end

local function reset_room_stats()
    g_run_stats.room_time = 0
    g_run_stats.room_deaths = 0
    g_run_stats.room_shots_fired = 0
    g_run_stats.room_shots_hit = 0
    g_run_stats.room_shot_ricochets = 0
    g_run_stats.room_shot_bank_kills = 0
    g_run_stats.room_jumps = 0
    g_run_stats.room_grapples_fired = 0
    g_run_stats.room_grapple_latches = 0
    g_run_stats.room_dashes = 0
    g_run_stats.room_rolls = 0
    g_run_stats.room_crouch_time = 0
    g_run_stats.room_distance = 0
    g_run_stats.room_checkpoints = 0
    g_run_stats.room_last_checkpoint_at = nil
    g_run_stats.room_last_checkpoint_id = nil
    g_run_stats.room_respawns = 0
    g_run_stats.room_last_respawn_latency = nil
    g_run_stats.room_gate_entered = false
    g_run_stats.room_first_meaningful_action_at = nil
    g_run_stats.room_environment_label_seen = false
    g_run_stats.room_environment_label = nil
    g_run_stats.room_removed_ability_notice_shown = false
    g_run_stats.room_encounter_complete = false
    g_run_stats.room_notice_events = 0
end

local function mark_meaningful_action()
    if not g_run_stats then
        return
    end
    if g_run_stats.first_meaningful_action_at == nil then
        g_run_stats.first_meaningful_action_at = g_run_stats.total_time
    end
    if g_run_stats.room_first_meaningful_action_at == nil then
        g_run_stats.room_first_meaningful_action_at = g_run_stats.room_time
    end
end

local function record_removed_ability_notice()
    if not g_run_stats then
        return
    end
    g_run_stats.room_removed_ability_notice_shown = true
    g_run_stats.room_notice_events = (g_run_stats.room_notice_events or 0) + 1
    mark_meaningful_action()
end

local function record_environment_label(label)
    if not g_run_stats or not label or label == "" then
        return
    end
    g_run_stats.room_environment_label_seen = true
    g_run_stats.room_environment_label = label
    mark_meaningful_action()
end

local function trigger_hitstop(duration)
    g_hitstop_timer = math.max(g_hitstop_timer or 0, duration or 0)
end

g_trigger_hitstop = trigger_hitstop

local function record_encounter_complete()
    if not g_run_stats then
        return
    end
    g_run_stats.room_encounter_complete = true
    mark_meaningful_action()
end

local function record_checkpoint_activated(checkpoint)
    if not g_run_stats or not checkpoint then
        return
    end
    g_run_stats.checkpoints = (g_run_stats.checkpoints or 0) + 1
    g_run_stats.room_checkpoints = (g_run_stats.room_checkpoints or 0) + 1
    g_run_stats.room_last_checkpoint_at = g_run_stats.room_time
    g_run_stats.room_last_checkpoint_id = checkpoint.id
    mark_meaningful_action()
end

local function record_gate_entered()
    if not g_run_stats then
        return
    end
    g_run_stats.room_gate_entered = true
    mark_meaningful_action()
end

-- Ambient drone generation per circle
local function generate_drone(circle_id)
    if g_ambient_source then
        g_ambient_source:stop()
        g_ambient_source = nil
    end

    local sample_rate = 44100
    local duration = 4.0
    local sample_count = math.floor(sample_rate * duration)
    local sound_data = love.sound.newSoundData(sample_count, sample_rate, 16, 1)

    -- Different drone character per circle
    local base_freq = 55  -- A1
    local detune = 0.5
    local volume = 0.08

    if circle_id == "lust" then
        base_freq = 62    -- B1, slightly unsettled
        detune = 1.2
        volume = 0.1
    end

    for i = 0, sample_count - 1 do
        local t = i / sample_rate
        -- Two detuned sine waves + slow LFO modulation
        local lfo = math.sin(t * 0.3 * math.pi * 2) * 0.3
        local osc1 = math.sin(t * base_freq * math.pi * 2) * 0.5
        local osc2 = math.sin(t * (base_freq + detune) * math.pi * 2) * 0.5
        local osc3 = math.sin(t * (base_freq * 1.5) * math.pi * 2) * 0.15
        -- Fade loop edges for seamless repeat
        local env = 1
        local fade = 0.1 * duration
        if t < fade then
            env = t / fade
        elseif t > duration - fade then
            env = (duration - t) / fade
        end
        local sample = (osc1 + osc2 + osc3) * (1 + lfo) * volume * env
        sound_data:setSample(i, math.max(-1, math.min(1, sample)))
    end

    local source = love.audio.newSource(sound_data)
    source:setLooping(true)
    source:setVolume(volume)
    source:play()
    g_ambient_source = source
end

-- Stop ambient drone
local function stop_drone()
    if g_ambient_source then
        g_ambient_source:stop()
        g_ambient_source = nil
        g_ambient_circle_id = nil
    end
end

-- Trial completion checking
local function check_trial_completion()
    if not g_current_scene or g_trial_completed then
        return false
    end

    local completion = g_current_scene.completion or "gate"

    if completion == "gate" then
        -- Legacy: walk into gate with enough fragments
        return g_level:check_gate_collision(g_player)
            and g_player.fragments >= g_level.fragments_required
            and g_encounters:is_complete()
    elseif completion == "reach_zone" then
        local zone = g_current_scene.reach_zone
        if not zone then return false end
        return g_player.x + g_player.width > zone.x
            and g_player.x < zone.x + zone.width
            and g_player.y + g_player.height > zone.y
            and g_player.y < zone.y + zone.height
    elseif completion == "kill_all" then
        return g_encounters:is_complete() and #g_enemies.items == 0
    elseif completion == "survive" then
        -- Survive until timer hits 0 — completion triggered by timer expiry
        return false
    elseif completion == "collect_all" then
        return g_player.fragments >= g_level.fragments_required
    end

    return false
end

-- Grade calculation
local function calculate_grade(scene, room_time, deaths)
    local timer = scene.trial_timer or 0
    if timer <= 0 then
        -- No timer = grade on deaths only
        if deaths == 0 then return "S", 300 end
        if deaths == 1 then return "A", 200 end
        if deaths <= 3 then return "B", 100 end
        return "C", 50
    end

    local time_ratio = room_time / timer
    if deaths == 0 and time_ratio < 0.5 then return "S", 300 end
    if deaths == 0 and time_ratio < 0.75 then return "A", 200 end
    if deaths <= 1 then return "B", 100 end
    return "C", 50
end

-- Hard-cut transition: flash black then advance
local function trigger_hard_cut()
    if g_hard_cut_pending then return end
    g_hard_cut_pending = true
    g_trial_completed = true
    g_hard_cut_timer = 0

    -- Calculate grade
    local grade, points = calculate_grade(
        g_current_scene,
        g_run_stats.room_time,
        g_run_stats.room_deaths
    )
    g_trial_score = {
        grade = grade,
        points = points,
        time = g_run_stats.room_time,
        deaths = g_run_stats.room_deaths,
        timer = 0.65, -- How long to show the grade
    }
    g_circle_score = g_circle_score + points
end

local function resolve_scene()
    if g_current_flow then
        return SceneContract.normalize(g_current_flow.rooms[g_current_room_index])
    end
    return SceneContract.normalize(circle_data[g_current_circle + 1])
end

local function load_scene(scene)
    g_current_scene = scene
    g_level = Level:new(scene, g_services)
    if g_services then
        g_services:set_scene(scene, g_level)
        g_services:set_progress(
            g_game_mode,
            g_current_flow and g_current_room_index or (g_current_circle + 1),
            g_current_flow and #g_current_flow.rooms or #circle_data
        )
    end
    g_hitstop_timer = 0
    g_encounters:load_scene(scene)
    g_player:apply_scene_rules(scene)
    g_player:set_start_pos(g_level.start_pos)
    g_respawn_point = g_level:get_respawn_point()
    g_player:respawn(g_respawn_point)
    g_player.fragments = 0
    g_collectibles:respawn()
    g_camera:reset()
    g_effects:reset()
    g_background:set_scene(scene)

    -- Trial system init
    g_trial_timer = scene.trial_timer or 0
    g_trial_active = g_trial_timer > 0
    g_trial_completed = false
    g_hard_cut_pending = false
    g_hard_cut_timer = 0
    g_trial_score = nil

    -- Ambient drone per circle
    local circle_id = scene.circle_id
    if circle_id and g_game_mode == "campaign" then
        if g_ambient_circle_id ~= circle_id then
            generate_drone(circle_id)
            g_ambient_circle_id = circle_id
        end
    elseif not circle_id then
        stop_drone()
    end

    g_ui:show_scene_intro(
        scene,
        g_current_flow and g_current_room_index or 1,
        g_current_flow and #g_current_flow.rooms or 1
    )
    reset_room_stats()
    if g_autoplay then
        g_autoplay:on_scene_loaded(scene, g_current_room_index)
    end
end

local function advance_current_room()
    if g_current_flow then
        local completed_slice = false
        g_run_stats.rooms_cleared = g_run_stats.rooms_cleared + 1

        -- Check if we're crossing a circle boundary
        local old_circle = g_current_scene and g_current_scene.circle_id
        g_current_room_index = g_current_room_index + 1
        if g_current_room_index > #g_current_flow.rooms then
            g_current_room_index = 1
            completed_slice = true
        end

        local next_scene = resolve_scene()
        local new_circle = next_scene and next_scene.circle_id

        -- Reset circle score when entering a new circle
        if old_circle and new_circle and old_circle ~= new_circle then
            g_circle_score = 0
        end

        load_scene(next_scene)
        if completed_slice then
            g_circle_score = 0
            g_ui:show_banner(
                "ASCENT COMPLETE",
                string.format("Final score: %d. Looping.", g_circle_score),
                { 0.95, 0.62, 0.22 },
                2.0
            )
        end
        return
    end

    g_current_circle = g_current_circle + 1
    if g_current_circle >= #circle_data then
        g_current_circle = 0
    end
    load_scene(resolve_scene())
end

local function quick_reset()
    g_run_stats.resets = g_run_stats.resets + 1
    load_scene(resolve_scene())
end

local function select_flow_room(index)
    if not g_current_flow then
        return
    end

    local clamped = math.max(1, math.min(#g_current_flow.rooms, index))
    if clamped ~= g_current_room_index then
        g_current_room_index = clamped
        load_scene(resolve_scene())
    end
end

local function respawn_player()
    local latency = g_death_timer
    g_death_timer = 0
    g_respawn_point = g_level:get_respawn_point()
    g_player:respawn(g_respawn_point)
    g_collectibles:respawn()
    g_camera:reset()
    g_effects:reset()
    g_effects:spawn("jump_dust", g_respawn_point.x + g_player.width / 2, g_respawn_point.y + g_player.height)
    g_ui:trigger_flash()
    if g_game_mode == "proving_ground" and g_level.recovery_hint and g_level.recovery_hint ~= "" then
        g_ui:show_story_callout(g_level.scene_title, g_level.recovery_hint, g_level.accent_color, 2.2)
    else
        g_ui:show_banner(g_level.scene_title, "Back in. Reclaim the line.", g_level.accent_color, 1.1)
    end
    if g_run_stats then
        g_run_stats.respawns = (g_run_stats.respawns or 0) + 1
        g_run_stats.room_respawns = (g_run_stats.room_respawns or 0) + 1
        g_run_stats.last_respawn_latency = latency
        g_run_stats.room_last_respawn_latency = latency
    end
end

function love.load(args)
    math.randomseed(os.time())
    love.graphics.setDefaultFilter("nearest", "nearest")

    if has_flag(args, "--preview-player") then
        g_preview_mode = true
        love.window.setMode(
            g_native_width * 3,
            g_native_height * 3,
            { resizable = true, vsync = true }
        )
        love.window.setTitle("Infernal Ascent - Player Preview")
        g_preview = PreviewPlayer:new()
        g_preview:load()
        return
    end

    apply_game_window_mode(args)

    g_main_canvas = love.graphics.newCanvas(g_native_width, g_native_height)

    g_services = RuntimeServices:new()
    g_sfx = Sfx:new()
    g_effects = Effects:new()
    g_ui = UI:new(g_services)
    g_collectibles = Collectibles:new(g_services)
    g_enemies = Enemies:new(g_services)
    g_background = Background:new()
    g_player = Player:new(g_services)
    g_projectiles = Projectiles:new(g_services)
    g_camera = Camera:new(g_player)
    g_encounters = EncounterController:new(g_services)
    g_services.sfx = g_sfx
    g_services.effects = g_effects
    g_services.ui = g_ui
    g_services.collectibles = g_collectibles
    g_services.enemies = g_enemies
    g_services.background = g_background
    g_services.player = g_player
    g_services.projectiles = g_projectiles
    g_services.camera = g_camera
    g_services.encounters = g_encounters
    g_services.trigger_hitstop = trigger_hitstop
    g_services.mark_meaningful_action = mark_meaningful_action
    g_services.record_removed_ability_notice = record_removed_ability_notice
    g_services.record_environment_label = record_environment_label
    g_services.record_encounter_complete = record_encounter_complete
    g_services.record_checkpoint_activated = record_checkpoint_activated
    g_services.record_gate_entered = record_gate_entered
    g_services.advance_current_room = advance_current_room
    g_services.quick_reset = quick_reset
    g_services.respawn_player = respawn_player
    g_services.select_flow_room = select_flow_room
    g_services.input_override = g_input_override
    g_services.debug_overlay = g_debug_overlay
    init_run_stats()

    if has_flag(args, "--autoplay") then
        install_input_override()
        g_autoplay = Autoplay:new(
            g_services,
            get_flag_value(args, "--autoplay-report="),
            get_flag_value(args, "--autoplay-variant=") or "a"
        )
        g_services.autoplay = g_autoplay
        if not has_flag(args, "--autoplay-audio") then
            g_sfx:set_muted(true)
        end
        if not has_flag(args, "--autoplay-no-screenshots") then
            g_autoplay:set_screenshot_dir(get_flag_value(args, "--autoplay-shots=") or "tmp/autoplay-shots")
        end
    end

    local qa_shot_path = get_flag_value(args, "--qa-shot=")
    if qa_shot_path then
        g_qa_capture = {
            path = absolute_path(qa_shot_path),
            delay = tonumber(get_flag_value(args, "--qa-shot-delay=") or "0.25") or 0.25,
            requested = false,
            completed = false,
        }
    end

    if has_flag(args, "--campaign") then
        g_game_mode = "campaign"
        g_current_flow = campaign
        g_current_room_index = math.max(
            1,
            math.min(#g_current_flow.rooms, tonumber(get_flag_value(args, "--room=") or "1") or 1)
        )
    elseif has_flag(args, "--circles") then
        g_game_mode = "circles"
        g_current_flow = nil
        local circle_arg = tonumber(get_flag_value(args, "--circle=") or "0") or 0
        g_current_circle = math.max(0, math.min(#circle_data - 1, circle_arg))
    else
        g_game_mode = "proving_ground"
        g_current_flow = proving_ground
        g_current_room_index = math.max(
            1,
            math.min(#g_current_flow.rooms, tonumber(get_flag_value(args, "--room=") or "1") or 1)
        )
    end

    g_background:load()
    load_scene(resolve_scene())
end

function love.update(dt)
    if g_preview_mode then
        g_preview:update(dt)
        return
    end

    if g_autoplay then
        g_autoplay:update(dt)
    end

    if g_hitstop_timer > 0 then
        g_hitstop_timer = math.max(0, g_hitstop_timer - dt)
        g_camera:update(dt)
        g_effects:update(dt)
        g_background:update(dt)
        g_ui:update(dt)
        return
    end

    g_run_stats.total_time = g_run_stats.total_time + dt
    g_run_stats.room_time = g_run_stats.room_time + dt
    g_run_stats.distance = g_run_stats.distance + math.abs(g_player.vx) * dt
    g_run_stats.room_distance = g_run_stats.room_distance + math.abs(g_player.vx) * dt
    if g_player.is_crouching then
        g_run_stats.crouch_time = g_run_stats.crouch_time + dt
        g_run_stats.room_crouch_time = g_run_stats.room_crouch_time + dt
    end

    if g_player.is_dead then
        g_death_timer = g_death_timer + dt
        if g_death_timer > 0.16 then
            respawn_player()
        end
        g_camera:update(dt)
        g_effects:update(dt)
        g_background:update(dt)
        g_ui:update(dt)
        return
    end

    -- Hard-cut transition in progress
    if g_hard_cut_pending then
        g_hard_cut_timer = g_hard_cut_timer + dt
        -- Show grade for a beat, then cut
        if g_trial_score then
            g_trial_score.timer = g_trial_score.timer - dt
        end
        local grade_done = not g_trial_score or g_trial_score.timer <= 0
        if g_hard_cut_timer > 0.08 and grade_done then
            g_hard_cut_pending = false
            g_hard_cut_timer = 0
            g_trial_score = nil
            record_gate_entered()
            advance_current_room()
        end
        g_camera:update(dt)
        g_ui:update(dt)
        return
    end

    g_level:update(dt, g_player)
    g_player:update(dt, g_level)
    g_encounters:update(dt, g_player)
    g_encounters:constrain_player(g_player)
    g_camera:update(dt)
    g_effects:update(dt)
    g_background:update(dt)
    g_collectibles:update(dt, g_player)
    g_enemies:update(dt, g_level, g_player)
    g_ui:update(dt)
    g_projectiles:update(dt, g_level, g_enemies)

    -- Trial timer countdown
    if g_trial_active and not g_trial_completed then
        g_trial_timer = g_trial_timer - dt
        if g_trial_timer <= 0 then
            g_trial_timer = 0
            local completion = g_current_scene and g_current_scene.completion or "gate"
            if completion == "survive" then
                -- Survived! Trigger completion
                local use_hard_cut = g_current_scene and g_current_scene.hard_cut
                if use_hard_cut then
                    trigger_hard_cut()
                else
                    g_trial_completed = true
                    record_gate_entered()
                    advance_current_room()
                end
            else
                -- Ran out of time = death
                g_player:take_damage()
                if not g_player.is_dead then
                    g_player:die()
                end
            end
        end
    end

    -- Checkpoint collision (legacy rooms)
    local checkpoint = g_level:check_checkpoint_collision(g_player)
    if checkpoint then
        g_respawn_point = g_level:get_respawn_point()
        g_effects:rumble(checkpoint.x + checkpoint.width / 2, checkpoint.y + checkpoint.height)
        g_ui:trigger_flash()
        g_ui:show_banner(g_level.scene_title, "Checkpoint.", g_level.accent_color, 0.8)
    end

    if g_level:handle_hazards_collision(g_player) then
        g_player:take_damage()
    end

    -- Completion condition check
    if check_trial_completion() then
        local use_hard_cut = g_current_scene and g_current_scene.hard_cut
        if use_hard_cut then
            trigger_hard_cut()
        else
            record_gate_entered()
            advance_current_room()
        end
    end
end

function love.draw()
    if g_preview_mode then
        g_preview:draw()
        return
    end

    love.graphics.setCanvas(g_main_canvas)
    love.graphics.clear()

    g_background:draw()
    g_camera:attach()
    g_level:draw()
    g_encounters:draw()
    g_player:draw()
    g_effects:draw()
    g_collectibles:draw()
    g_enemies:draw()
    g_projectiles:draw()
    g_camera:detach()

    local hide_standard_hud = g_ui:should_hide_standard_hud()
    if not hide_standard_hud then
        g_ui:draw_fragments(
            g_player.fragments,
            g_level.fragments_required,
            g_level.scene_title,
            g_level.room_type
        )
        g_ui:draw_health(g_player.health, g_player.max_health)
    end
    g_ui:draw_banner()
    g_ui:draw_chapter_card()
    g_ui:draw_story_callout()
    if not hide_standard_hud then
        g_ui:draw_encounter_status()
        g_ui:draw_campaign_progress(
            g_current_flow and #g_current_flow.rooms or nil,
            g_current_room_index
        )
    end
    -- Trial timer HUD
    if g_trial_active and not g_trial_completed and g_trial_timer > 0 then
        g_ui:draw_trial_timer(g_trial_timer, g_current_scene and g_current_scene.trial_timer or 0)
    end

    -- Trial rule flash
    if g_current_scene and g_current_scene.trial_rule and g_current_scene.trial_rule ~= "" then
        g_ui:draw_trial_rule(g_current_scene.trial_rule, g_run_stats.room_time)
    end

    -- Circle score
    if g_game_mode == "campaign" and g_circle_score > 0 then
        g_ui:draw_circle_score(g_circle_score)
    end

    g_ui:draw_debug(
        g_level,
        g_run_stats,
        g_current_flow and #g_current_flow.rooms or nil,
        g_current_room_index,
        g_debug_overlay
    )

    -- Hard-cut black flash overlay
    if g_hard_cut_pending then
        love.graphics.setColor(0, 0, 0, 1)
        love.graphics.rectangle("fill", 0, 0, g_native_width, g_native_height)

        -- Grade flash
        if g_trial_score and g_trial_score.timer > 0 then
            local accent = g_current_scene and g_current_scene.accent_color or {0.92, 0.55, 0.18}
            local alpha = math.min(1, g_trial_score.timer / 0.3)

            -- Grade letter
            love.graphics.setColor(accent[1], accent[2], accent[3], alpha)
            love.graphics.printf(
                g_trial_score.grade,
                0, g_native_height / 2 - 30, g_native_width, "center"
            )

            -- Points
            love.graphics.setColor(0.85, 0.88, 0.95, alpha * 0.8)
            love.graphics.printf(
                string.format("+%d", g_trial_score.points),
                0, g_native_height / 2 - 10, g_native_width, "center"
            )

            -- Time and deaths
            love.graphics.setColor(0.6, 0.62, 0.7, alpha * 0.6)
            love.graphics.printf(
                string.format("%.1fs  %d deaths", g_trial_score.time, g_trial_score.deaths),
                0, g_native_height / 2 + 8, g_native_width, "center"
            )
        end
        love.graphics.setColor(1, 1, 1, 1)
    end

    love.graphics.setCanvas()

    local scale = math.min(
        love.graphics.getWidth() / g_native_width,
        love.graphics.getHeight() / g_native_height
    )
    love.graphics.draw(g_main_canvas, 0, 0, 0, scale, scale)

    if g_autoplay then
        g_autoplay:process_screenshot()
    end
    request_qa_capture_if_ready()
end

function love.keypressed(key)
    if g_preview_mode then
        g_preview:keypressed(key)
        return
    end

    if key == "f3" then
        g_debug_overlay = not g_debug_overlay
        if g_services then
            g_services.debug_overlay = g_debug_overlay
        end
        return
    end

    if (g_game_mode == "proving_ground" or g_game_mode == "campaign") and key == "tab" then
        quick_reset()
        return
    end

    if g_game_mode == "proving_ground" and g_current_flow then
        if key == "]" then
            select_flow_room(g_current_room_index + 1)
            return
        elseif key == "[" then
            select_flow_room(g_current_room_index - 1)
            return
        end
    end

    g_player:keypressed(key)
    if key == "r" then
        love.event.quit("restart")
    end
end

function love.keyreleased(key)
    if g_preview_mode then
        return
    end

    if g_player and g_player.keyreleased then
        g_player:keyreleased(key)
    end
end

function love.mousepressed(x, y, button)
    if g_preview_mode then
        return
    end

    local scale = math.min(
        love.graphics.getWidth() / g_native_width,
        love.graphics.getHeight() / g_native_height
    )
    local canvas_x = x / scale
    local canvas_y = y / scale
    local world_x, world_y = g_camera:to_world(canvas_x, canvas_y)

    if button == 1 then
        g_player:fire_weapon(world_x, world_y)
    elseif button == 2 then
        g_player:fire_grapple(world_x, world_y)
    end
end

g_select_flow_room = select_flow_room
g_mark_meaningful_action = mark_meaningful_action
g_record_removed_ability_notice = record_removed_ability_notice
g_record_environment_label = record_environment_label
g_record_encounter_complete = record_encounter_complete
