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

g_native_width, g_native_height = 480, 270
g_main_canvas = nil

circle_data = require("circles")
local campaign = require("campaign")
local proving_ground = require("proving_ground")
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
        jumps = 0,
        room_jumps = 0,
        grapples_fired = 0,
        room_grapples_fired = 0,
        grapple_latches = 0,
        room_grapple_latches = 0,
        distance = 0,
        room_distance = 0,
        rooms_cleared = 0,
    }
end

local function reset_room_stats()
    g_run_stats.room_time = 0
    g_run_stats.room_deaths = 0
    g_run_stats.room_shots_fired = 0
    g_run_stats.room_shots_hit = 0
    g_run_stats.room_jumps = 0
    g_run_stats.room_grapples_fired = 0
    g_run_stats.room_grapple_latches = 0
    g_run_stats.room_distance = 0
end

local function resolve_scene()
    if g_current_flow then
        return g_current_flow.rooms[g_current_room_index]
    end
    return circle_data[g_current_circle + 1]
end

local function load_scene(scene)
    g_current_scene = scene
    g_level = Level:new(scene)
    g_player:apply_scene_rules(scene)
    g_player:set_start_pos(g_level.start_pos)
    g_respawn_point = g_level:get_respawn_point()
    g_player:respawn(g_respawn_point)
    g_player.fragments = 0
    g_collectibles:respawn()
    g_camera:reset()
    g_effects:reset()
    g_background:set_scene(scene)
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
        g_current_room_index = g_current_room_index + 1
        if g_current_room_index > #g_current_flow.rooms then
            g_current_room_index = 1
            completed_slice = true
        end
        load_scene(resolve_scene())
        if completed_slice then
            g_ui:show_scene_intro({
                title = "SLICE COMPLETE",
                subtitle = "Looping back to the first room.",
                accent_color = { 0.95, 0.62, 0.22 },
                mode = "campaign",
                room_type = "transition",
                removed_ability = "none",
                environment_hook = "Vertical slice complete",
            }, #g_current_flow.rooms, #g_current_flow.rooms)
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
    g_death_timer = 0
    g_respawn_point = g_level:get_respawn_point()
    g_player:respawn(g_respawn_point)
    g_collectibles:respawn()
    g_camera:reset()
    g_effects:reset()
end

function love.load(args)
    math.randomseed(os.time())
    love.graphics.setDefaultFilter("nearest", "nearest")

    if has_flag(args, "--preview-player") then
        g_preview_mode = true
        love.window.setMode(
            g_native_width * 2,
            g_native_height * 2,
            { resizable = true, vsync = true }
        )
        love.window.setTitle("Infernal Ascent - Player Preview")
        g_preview = PreviewPlayer:new()
        g_preview:load()
        return
    end

    love.window.setMode(g_native_width * 3, g_native_height * 3, { resizable = true, vsync = true })
    love.window.setTitle("Infernal Ascent")

    g_main_canvas = love.graphics.newCanvas(g_native_width, g_native_height)

    g_sfx = Sfx:new()
    g_effects = Effects:new()
    g_ui = UI:new()
    g_collectibles = Collectibles:new()
    g_enemies = Enemies:new()
    g_background = Background:new()
    g_player = Player:new()
    g_projectiles = Projectiles:new()
    g_camera = Camera:new(g_player)
    init_run_stats()

    if has_flag(args, "--autoplay") then
        install_input_override()
        g_autoplay = Autoplay:new(get_flag_value(args, "--autoplay-report="))
        if not has_flag(args, "--autoplay-audio") then
            g_sfx:set_muted(true)
        end
        if not has_flag(args, "--autoplay-no-screenshots") then
            g_autoplay:set_screenshot_dir(get_flag_value(args, "--autoplay-shots=") or "tmp/autoplay-shots")
        end
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

    g_run_stats.total_time = g_run_stats.total_time + dt
    g_run_stats.room_time = g_run_stats.room_time + dt
    g_run_stats.distance = g_run_stats.distance + math.abs(g_player.vx) * dt
    g_run_stats.room_distance = g_run_stats.room_distance + math.abs(g_player.vx) * dt

    if g_player.is_dead then
        g_death_timer = g_death_timer + dt
        if g_death_timer > 0.75 then
            respawn_player()
        end
        g_camera:update(dt)
        g_effects:update(dt)
        g_background:update(dt)
        g_ui:update(dt)
        return
    end

    g_level:update(dt)
    g_player:update(dt, g_level)
    g_camera:update(dt)
    g_effects:update(dt)
    g_background:update(dt)
    g_collectibles:update(dt, g_player)
    g_enemies:update(dt, g_level, g_player)
    g_ui:update(dt)
    g_projectiles:update(dt, g_level, g_enemies)

    local checkpoint = g_level:check_checkpoint_collision(g_player)
    if checkpoint then
        g_respawn_point = g_level:get_respawn_point()
        g_ui:show_banner(g_level.scene_title, "Checkpoint secured.", g_level.accent_color)
    end

    if g_level:handle_hazards_collision(g_player) then
        g_player:take_damage()
    end

    if
        g_level:check_gate_collision(g_player)
        and g_player.fragments >= g_level.fragments_required
    then
        advance_current_room()
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
    g_player:draw()
    g_effects:draw()
    g_collectibles:draw()
    g_enemies:draw()
    g_projectiles:draw()
    g_camera:detach()

    g_ui:draw_fragments(g_player.fragments, g_level.fragments_required, g_level.scene_title)
    g_ui:draw_health(g_player.health, g_player.max_health)
    g_ui:draw_banner()
    g_ui:draw_chapter_card()
    g_ui:draw_campaign_progress(
        g_current_flow and #g_current_flow.rooms or nil,
        g_current_room_index
    )
    g_ui:draw_debug(
        g_level,
        g_run_stats,
        g_current_flow and #g_current_flow.rooms or nil,
        g_current_room_index,
        g_debug_overlay
    )
    if g_player.is_dead then
        g_ui:draw_death_screen()
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
end

function love.keypressed(key)
    if g_preview_mode then
        g_preview:keypressed(key)
        return
    end

    if key == "f3" then
        g_debug_overlay = not g_debug_overlay
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
