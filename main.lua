-- Global instances of our game objects
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

-- Game state
g_current_circle = 0
g_distort_intensity = 0.002 -- Subtle but noticeable for depth
g_death_timer = 0 -- Timer for respawn delay

-- Canvases and Shaders
g_native_width, g_native_height = 480, 270
g_main_canvas = nil
g_refraction_canvas = nil
g_noise_texture = nil
g_refraction_shader = nil
g_tile_shader = nil
g_post_shader = nil

-- Engine Modules
circle_data = require('circles')
Player = require('player')
Level = require('level')
Camera = require('camera')
Sfx = require('sfx')
Effects = require('effects')
UI = require('ui')
Background = require('background')
Collectibles = require('collectibles')
Enemies = require('enemies')
Projectiles = require('projectiles')

local function generate_noise(image_data)
    for y = 0, image_data:getHeight() - 1 do
        for x = 0, image_data:getWidth() - 1 do
            local val = love.math.noise(x / 16, y / 16)
            image_data:setPixel(x, y, val, val, val, 1)
        end
    end
    g_noise_texture = love.graphics.newImage(image_data)
    g_noise_texture:setFilter("nearest", "nearest")
    g_noise_texture:setWrap("repeat", "repeat")
end

function love.load()
    math.randomseed(os.time())
    love.window.setMode(g_native_width * 3, g_native_height * 3, {resizable=true, vsync=true})
    love.window.setTitle("Infernal Ascent")
    love.graphics.setDefaultFilter("nearest", "nearest")
    
    -- Graphics Objects
    g_main_canvas = love.graphics.newCanvas(g_native_width, g_native_height)
    g_refraction_canvas = love.graphics.newCanvas(g_native_width, g_native_height)
    local noise_data = love.image.newImageData(256, 256)
    generate_noise(noise_data)
    g_refraction_shader = love.graphics.newShader('refraction.glsl')
    g_tile_shader = love.graphics.newShader('tile.glsl')
    g_post_shader = love.graphics.newShader('post_processor.glsl')

    -- Game Modules (Order is critical)
    g_sfx = Sfx:new()
    g_effects = Effects:new()
    g_ui = UI:new()
    g_collectibles = Collectibles:new()
    g_enemies = Enemies:new()
    g_background = Background:new()
    g_player = Player:new()
    g_projectiles = Projectiles:new()
    g_camera = Camera:new(g_player)
    g_level = Level:new(circle_data[1].map)
    
    -- Final Setup
    g_player:set_start_pos(g_level.start_pos)
    g_background:load()
end

function love.update(dt)
    -- Handle death and respawn
    if g_player.is_dead then
        g_death_timer = g_death_timer + dt
        if g_death_timer > 1.5 then
            g_death_timer = 0
            g_player:respawn(g_level.start_pos)
            g_collectibles:respawn()
            g_camera:reset()
        end
        -- Still update camera and effects during death
        g_camera:update(dt)
        g_effects:update(dt)
        g_background:update(dt)
        return
    end

    g_player:update(dt, g_level)
    g_level:update(dt)
    g_camera:update(dt)
    g_effects:update(dt)
    g_background:update(dt)
    g_collectibles:update(dt, g_player)
    g_enemies:update(dt, g_level, g_player)
    g_ui:update(dt)
    g_projectiles:update(dt, g_level, g_enemies)

    -- Check for hazard collision
    if g_level:handle_hazards_collision(g_player) then
        g_player:take_damage()
    end

    if g_level:check_gate_collision(g_player) and g_player.fragments >= g_level.fragments_required then
        g_current_circle = g_current_circle + 1
        if g_current_circle >= #circle_data then g_current_circle = 0 end
        local next_map = circle_data[g_current_circle + 1].map
        g_level = Level:new(next_map)
        g_player:respawn(g_level.start_pos)
        g_player.fragments = 0 -- Reset fragments for new level
    end
end

function love.draw()
    -- Render the game world to a temporary canvas for effects
    love.graphics.setCanvas(g_refraction_canvas)
    love.graphics.clear(0, 0, 0, 0) -- Clear with transparency
    g_camera:attach()
    
    love.graphics.setShader(g_tile_shader)
    g_level:draw()
    love.graphics.setShader()

    g_player:draw()
    g_effects:draw()
    g_collectibles:draw()
    g_enemies:draw()
    g_projectiles:draw()
    
    g_camera:detach()
    love.graphics.setCanvas() -- Done with refraction canvas

    -- Now, build the final scene on the main low-res canvas
    love.graphics.setCanvas(g_main_canvas)
    love.graphics.clear()

    -- 1. Draw the background
    g_background:draw()

    -- 2. Draw the processed game world on top, with shaders
    love.graphics.setShader(g_post_shader)
    love.graphics.draw(g_refraction_canvas)
    love.graphics.setShader()
    
    -- 3. Draw the UI
    g_ui:draw_fragments(g_player.fragments, g_level.fragments_required, g_current_circle)
    g_ui:draw_health(g_player.health, g_player.max_health)

    -- Draw death overlay
    if g_player.is_dead then
        g_ui:draw_death_screen()
    end

    -- FINISH RENDERING, SWITCH BACK TO THE SCREEN
    love.graphics.setCanvas()

    -- Finally, scale the completed canvas up to the window size
    local scale = math.min(love.graphics.getWidth() / g_native_width, love.graphics.getHeight() / g_native_height)
    love.graphics.draw(g_main_canvas, 0, 0, 0, scale, scale)
end

function love.keypressed(key)
    g_player:keypressed(key)
    if key == 'r' then
        love.event.quit("restart")
    end
end

function love.mousepressed(x, y, button)
    -- Rescale mouse coordinates to the native resolution
    local scale = math.min(love.graphics.getWidth() / g_native_width, love.graphics.getHeight() / g_native_height)
    local canvas_x = x / scale
    local canvas_y = y / scale

    local world_x, world_y = g_camera:to_world(canvas_x, canvas_y)
    
    if button == 1 then
        g_player:fire_weapon(world_x, world_y)
    elseif button == 2 then
        g_player:fire_grapple(world_x, world_y)
    end
end 