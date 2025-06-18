-- Infernø: A tiny, tight, symbolic micro-Metroidvania
-- by Maxwell "mwelsh" Young
-- made with LÖVE

Player = require('player')
Level = require('level')
Camera = require('camera')
UI = require('ui')
Particles = require('particles')
Effects = require('effects')
Animation = require('animation')

-- These are intentionally global for access from other modules
g_current_circle = 0
g_player = nil
g_level = nil
g_camera = nil
g_ui = nil
g_particles = nil
g_game_state = "playing"
vignette_shader = nil

local abilities_to_lose = {"has_double_jump", "has_dash", "has_wall_cling"}

function check_collision(a, b)
    -- Simple AABB check
    return a.x < b.x + b.width and
           a.x + a.width > b.x and
           a.y < b.y + b.height and
           a.y + a.height > b.y
end

function advance_circle()
    g_current_circle = g_current_circle + 1
    local ability_lost = abilities_to_lose[g_current_circle]

    if ability_lost and g_player then
        g_player[ability_lost] = false
        print("Ability lost: " .. ability_lost)
    end
    
    game_reset()
end

function game_reset()
    g_level = Level:new()
    g_player:set_start_pos(g_level.start_pos)
    g_player.vx = 0
    g_player.vy = 0
    g_level:reset()
end

function love.load()
    g_player = Player:new()
    g_level = Level:new()
    g_player:set_start_pos(g_level.start_pos)
    g_camera = Camera:new(g_player)
    g_ui = UI:new()
    g_particles = Particles:new()
    g_effects = Effects:new(g_particles)

    -- For pixel-perfect scaling
    love.graphics.setDefaultFilter("nearest", "nearest")

    vignette_shader = love.graphics.newShader("vignette.glsl")
    
    love.window.setTitle("Infernal Ascent")
    love.window.setMode(800, 600, {resizable=true, vsync=true})
end

function love.update(dt)
    if g_game_state == "playing" then
        g_player:update(dt)
        g_level:update(dt)

        -- Physics Update: Resolve X and Y axes separately
        -- X-axis
        g_player.x = g_player.x + g_player.vx * dt
        g_level:handle_collisions_x(g_player)

        -- Y-axis
        g_player.y = g_player.y + g_player.vy * dt
        g_level:handle_collisions_y(g_player)

        g_camera:update(dt)
        g_particles:update(dt)
        if g_effects then g_effects:update(dt) end

        if g_level:check_gate_collision(g_player) then
            advance_circle()
        end
        if g_level:check_hazard_collision(g_player) then
            game_reset()
        end
    end
end

function love.draw()
    love.graphics.clear(g_level.background_color[1], g_level.background_color[2], g_level.background_color[3], 1)

    -- love.graphics.setShader(vignette_shader)
    g_camera:attach()

    g_level:draw()
    g_player:draw()
    g_particles:draw()

    g_camera:detach()
    -- love.graphics.setShader()
    
    g_ui:draw()
end

function love.keypressed(key)
    if key == "escape" then
        love.event.quit()
    end
    if key == "r" then
        game_reset()
    end
    g_player:keypressed(key)
end

function love.keyreleased(key)
    g_player:keyreleased(key)
end

function love.resize(w, h)
    g_camera:resize(w, h)
end 