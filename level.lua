Level = {}
Level.__index = Level

function Level:new()
    local self = setmetatable({}, Level)
    self.platforms = {}
    self.hazards = {}
    self.wind_areas = {}
    self.goo_platforms = {}
    self.geysers = {}
    self.background_color = {0.1, 0.1, 0.1, 1} -- Default dark grey
    self.platform_color = {1, 1, 1, 1} -- Default white

    if g_current_circle == 0 then
        -- Paradiso: All abilities
        self.start_pos = {x = 100, y = 500}
        self.platforms = {
            {0, 550, 300, 50},   -- Start Platform
            {500, 550, 300, 50}, -- End Platform
        }
        self.gate = {x = 750, y = 500, width = 20, height = 50}

    elseif g_current_circle == 1 then
        -- Limbo (No ability loss yet, just a simple level)
        self.start_pos = {x = 50, y = 500}
        self.platforms = {
            {0, 550, 150, 50},   -- Start
            {250, 500, 100, 20}, -- Hop 1
            {400, 450, 100, 20}, -- Hop 2
            {550, 500, 100, 20}, -- Hop 3
            {650, 450, 150, 50}, -- End
        }
        self.gate = {x = 780, y = 400, width = 20, height = 50}

    elseif g_current_circle == 2 then
        -- Lust (No Dash)
        self.start_pos = {x = 100, y = 500}
        self.background_color = {0.3, 0.1, 0.2, 1}
        self.platform_color = {0.8, 0.6, 0.7, 1}
        self.platforms = {
            {0, 550, 200, 50},
            {450, 550, 350, 50},
        }
        self.wind_areas = {
            {x = 200, y = 0, width = 250, height = 600, force_x = 150, force_y = 0}
        }
        self.gate = {x = 750, y = 500, width = 20, height = 50}
        if g_particles then
            g_particles:add_emitter({
                x = 200, y = 0, width = 250, height = 600, count = 100, rate = 50,
                particle_props = {
                    vx_min = 50, vx_max = 100, vy_min = -10, vy_max = 10,
                    life_min = 2, life_max = 4, color = {0.8, 0.6, 0.7, 1}
                }
            })
        end

    elseif g_current_circle == 3 then
        -- Gluttony (No Double Jump)
        self.start_pos = {x = 50, y = 100}
        self.background_color = {0.1, 0.2, 0.1, 1}
        self.platform_color = {0.4, 0.6, 0.4, 1}
        self.platforms = {
            {0, 150, 100, 50},
            {650, 450, 150, 50},
        }
        self.goo_platforms = {
            {200, 200, 50, 400}
        }
        self.geysers = {
            {x = 400, y = 530, width = 50, height = 20, force = -1200, interval = 3, duration = 1, timer = math.random() * 3, is_active = false}
        }
        self.gate = {x = 780, y = 400, width = 20, height = 50}

    else
        self.start_pos = {x = 400, y = 450}
        self.platforms = {{0, 550, 800, 50}}
        self.gate = {x = 750, y = 500, width = 20, height = 50}
    end

    return self
end

function Level:draw()
    love.graphics.setColor(self.platform_color)
    for _, p in ipairs(self.platforms) do
        love.graphics.rectangle("fill", p[1], p[2], p[3], p[4])
    end

    love.graphics.setColor(0.3, 0.5, 0.3)
    for _, p in ipairs(self.goo_platforms) do
        love.graphics.rectangle("fill", p[1], p[2], p[3], p[4])
    end

    for _, g in ipairs(self.geysers) do
        if g.is_active then love.graphics.setColor(0.5, 0.8, 0.5)
        else love.graphics.setColor(0.3, 0.5, 0.3) end
        love.graphics.rectangle("fill", g.x, g.y, g.width, g.height)
    end

    love.graphics.setColor(1, 0, 0)
    for _, h in ipairs(self.hazards) do
        love.graphics.rectangle("fill", h[1], h[2], h[3], h[4])
    end

    love.graphics.setColor(1, 0, 1)
    love.graphics.rectangle("fill", self.gate.x, self.gate.y, self.gate.width, self.gate.height)
end

local function check_collision(a, b)
    return a.x < b.x + b.width and a.x + a.width > b.x and a.y < b.y + b.height and a.y + a.height > b.y
end

function Level:update(dt)
    for _, g in ipairs(self.geysers) do
        g.timer = g.timer + dt
        if not g.is_active and g.timer > g.interval then
            g.is_active = true
            g.timer = 0
        elseif g.is_active and g.timer > g.duration then
            g.is_active = false
            g.timer = 0
        end
    end
end

function Level:handle_wind(player, dt)
    for _, area in ipairs(self.wind_areas) do
        local wind_zone = {x = area.x, y = area.y, width = area.width, height = area.height}
        if check_collision(player, wind_zone) then
            player.vx = player.vx + area.force_x * dt
            player.vy = player.vy + area.force_y * dt
        end
    end
end

function Level:check_hazard_collision(player)
    for _, h_rect in ipairs(self.hazards) do
        local hazard = {x = h_rect[1], y = h_rect[2], width = h_rect[3], height = h_rect[4]}
        if check_collision(player, hazard) then return true end
    end
    return false
end

function Level:check_gate_collision(player)
    return check_collision(player, self.gate)
end

function Level:reset()
    if g_particles then g_particles.emitters = {} end
end

function Level:handle_collisions(player)
    player.is_grounded = false
    player.on_wall = 0

    local on_goo_wall = false
    for _, p_rect in ipairs(self.goo_platforms) do
        local platform = {x = p_rect[1], y = p_rect[2], width = p_rect[3], height = p_rect[4]}
        if check_collision(player, platform) then
            if player.x + player.width > platform.x and player.x < platform.x + platform.width then
                if player.vy > 0 then
                    player.vy = 50
                    on_goo_wall = true
                    player.jumps_made = 2 -- Prevent any jumping from goo
                end
            end
        end
    end

    for _, p_rect in ipairs(self.platforms) do
        local platform = {x = p_rect[1], y = p_rect[2], width = p_rect[3], height = p_rect[4]}
        if check_collision(player, platform) then
            local player_center_x = player.x + player.width / 2
            local player_center_y = player.y + player.height / 2
            local platform_center_x = platform.x + platform.width / 2
            local platform_center_y = platform.y + platform.height / 2
            local overlap_x = (player.width / 2 + platform.width / 2) - math.abs(player_center_x - platform_center_x)
            local overlap_y = (player.height / 2 + platform.height / 2) - math.abs(player_center_y - platform_center_y)

            if overlap_x < overlap_y then
                if on_goo_wall then
                    -- Sticky, no horizontal pushback
                elseif player_center_x < platform_center_x then
                    player.x = platform.x - player.width
                    player.on_wall = -1
                else
                    player.x = platform.x + platform.width
                    player.on_wall = 1
                end
                if player.is_dashing then player.is_dashing = false; player.vx = 0 end
            else
                if player_center_y < platform_center_y then
                    local was_airborne = not player.is_grounded
                    player.y = platform.y - player.height
                    if player.vy >= 0 then
                        if was_airborne then g_effects:land(player.x + player.width/2, player.y + player.height) end
                        player.vy = 0
                        player.is_grounded = true
                        player.jumps_made = 0
                        if player.is_dashing then player.is_dashing = false; player.vy = 0 end
                    end
                else
                    player.y = platform.y + platform.height
                    player.vy = 0
                end
            end
        end
    end

    for _, g in ipairs(self.geysers) do
        if g.is_active then
            local geyser_rect = {x = g.x, y = g.y, width = g.width, height = g.height}
            if check_collision(player, geyser_rect) then
                player.vy = g.force
                player.is_grounded = false
            end
        end
    end
end

return Level