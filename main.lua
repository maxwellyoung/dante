local Player = require("player")
local Level = require("level")
local Particles = require("particles")
local Effects = require("effects")
local Animation = require("animation")
local UI = require("ui")
local Camera = require("camera")

-- These are intentionally global for access from other modules
g_current_circle = 0
g_particles = nil
g_effects = nil

local abilities_to_lose = {"has_double_jump", "has_dash", "has_wall_cling"}
local player, level, gameState, vignette_shader, camera -- These are local to main.lua

function advance_circle()
    g_current_circle = g_current_circle + 1
    local ability_lost = abilities_to_lose[g_current_circle]

    if ability_lost and player then
        player[ability_lost] = false
        print("Ability lost: " .. ability_lost)
    end
    
    game_reset()
end

function game_reset()
    if level then level:reset() end
    level = Level:new()
    if player and level and level.start_pos then
        player:reset_position(level.start_pos.x, level.start_pos.y)
    end
end

function love.load()
    g_particles = Particles:new()
    g_effects = Effects:new(g_particles)
    player = Player:new(0,0)
    g_current_circle = 0
    gameState = "playing"
    vignette_shader = love.graphics.newShader("vignette.glsl")
    camera = Camera:new(800, 600)
    game_reset()
end

function love.update(dt)
    if gameState == "playing" then
        if g_particles then g_particles:update(dt) end
        if level then level:update(dt) end
        if player then player:update(dt) end
        if level and player then level:handle_wind(player, dt) end
        if level and player then level:handle_collisions(player) end

        if level and player and level:check_hazard_collision(player) then
            gameState = "dead"
            player.death_timer = 1 -- Wait 1 second before reset
            return
        end
        if level and player and level:check_gate_collision(player) then
            advance_circle()
        end
    elseif gameState == "dead" then
        player.death_timer = player.death_timer - dt
        if player.death_timer <= 0 then
            gameState = "playing"
            game_reset()
        end
    end
end

function love.draw()
    -- Set the background color first, before any drawing happens.
    if level and level.background_color then
        love.graphics.setBackgroundColor(level.background_color)
    else
        love.graphics.setBackgroundColor(0.1, 0.1, 0.1) -- Fallback
    end

    -- Attach the camera to scale the game world
    camera:attach()
    love.graphics.setShader(vignette_shader)

    if level then level:draw() end
    if player then player:draw() end
    if g_particles then g_particles:draw() end

    love.graphics.setShader() -- Stop applying the shader
    camera:detach() -- Detach the camera before drawing UI

    -- UI elements are drawn outside the camera at native resolution
    if player and g_effects then g_effects:draw_ui(player) end

    if gameState == "dead" then
        love.graphics.setColor(0, 0, 0, 0.7)
        love.graphics.rectangle("fill", 0, 0, love.graphics.getWidth(), love.graphics.getHeight())
        love.graphics.setColor(1, 0, 0)
        love.graphics.printf("YOU DIED", 0, love.graphics.getHeight() / 2 - 20, love.graphics.getWidth(), "center")
        love.graphics.setColor(1, 1, 1)
    end

    if gameState == "paused" then
        love.graphics.setColor(0, 0, 0, 0.5)
        love.graphics.rectangle("fill", 0, 0, love.graphics.getWidth(), love.graphics.getHeight())
        love.graphics.setColor(1, 1, 1)
        love.graphics.printf("PAUSED", 0, love.graphics.getHeight() / 2 - 20, love.graphics.getWidth(), "center")
    end

    UI:draw()
end

function love.keypressed(key)
    if key == "escape" then
        if gameState == "playing" then
            gameState = "paused"
        else
            gameState = "playing"
        end
    end

    if key == "r" then
        game_reset()
    end

    if key == "f" then
        local is_fullscreen, fstype = love.window.getFullscreen()
        love.window.setFullscreen(not is_fullscreen, "desktop")
    end

    if gameState == "playing" and player then
        if key == "space" or key == "up" or key == "w" then
            player:jump()
        elseif key == "z" or key == "x" or key == "k" then
            player:dash()
        end
    end
end

function love.resize(w, h)
    if camera then
        camera:resize(w, h)
    end
end 