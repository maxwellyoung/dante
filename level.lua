Level = {}
Level.__index = Level

function Level:new()
    local self = setmetatable({}, Level)
    self.platforms = {}
    self.hazards = {}
    self.wind_areas = {}
    self.background_color = {0.1, 0.1, 0.1, 1} -- Default dark grey
    self.platform_color = {1, 1, 1, 1} -- Default white

    if g_current_circle == 0 then
        -- Paradiso: Requires double jump or dash to cross the gap
        self.start_pos = {x = 100, y = 500}
        self.platforms = {
            {0, 550, 300, 50},   -- Start Platform
            {500, 550, 300, 50}, -- End Platform
        }
        self.gate = {x = 750, y = 500, width = 20, height = 50}

    elseif g_current_circle == 1 then
        -- Limbo (No Double Jump): A path of single, precise jumps
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
        -- Lust (No Dash): Requires momentum control (sprinting)
        self.start_pos = {x = 100, y = 500}
        self.background_color = {0.3, 0.1, 0.2, 1} -- Moody purplish-red
        self.platform_color = {0.8, 0.6, 0.7, 1} -- Lighter, dusty rose

        self.platforms = {
            {0, 550, 200, 50},   -- Start
            -- A long gap that requires a running start to clear
            {450, 550, 350, 50}, -- End
        }
        self.wind_areas = {
            -- A wind zone pushing the player to the right
            {x = 200, y = 0, width = 250, height = 600, force_x = 150, force_y = 0}
        }
        self.gate = {x = 750, y = 500, width = 20, height = 50}

        -- Add a particle emitter for the wind
        if g_particles then
            g_particles:add_emitter({
                x = 200, y = 0, width = 250, height = 600,
                count = 100,
                rate = 50,
                particle_props = {
                    vx_min = 50, vx_max = 100,
                    vy_min = -10, vy_max = 10,
                    life_min = 2, life_max = 4,
                    color = {0.8, 0.6, 0.7, 1}
                }
            })
        end

    elseif g_current_circle == 3 then
        -- Gluttony (No Wall Cling): Requires careful drops and air control
        self.start_pos = {x = 50, y = 100}
        self.platforms = {
            -- A high starting platform with a long drop
            {0, 150, 100, 50},
            -- A series of small platforms to land on during the fall
            {250, 250, 50, 20},
            {150, 350, 50, 20},
            -- The final landing area
            {0, 550, 400, 50},
            {500, 550, 300, 50}
        }
        self.hazards = {
            {400, 530, 100, 20} -- Spikes in the gap on the floor
        }
        self.gate = {x = 750, y = 500, width = 20, height = 50}

    else
        -- Default/Fallback level
        self.start_pos = {x = 400, y = 450}
        self.platforms = {
            {0, 550, 800, 50}
        }
        self.gate = {x = 750, y = 500, width = 20, height = 50}
    end

    return self
end

function Level:draw()
    love.graphics.setColor(self.platform_color)
    for _, p in ipairs(self.platforms) do
        love.graphics.rectangle("fill", p[1], p[2], p[3], p[4])
    end

    love.graphics.setColor(1, 0, 0) -- Red for hazards
    for _, h in ipairs(self.hazards) do
        love.graphics.rectangle("fill", h[1], h[2], h[3], h[4])
    end

    -- Draw the gate in a different color
    love.graphics.setColor(1, 0, 1) -- Purple
    love.graphics.rectangle("fill", self.gate.x, self.gate.y, self.gate.width, self.gate.height)
end

-- AABB collision check
local function check_collision(a, b)
    return a.x < b.x + b.width and
           a.x + a.width > b.x and
           a.y < b.y + b.height and
           a.y + a.height > b.y
end

function Level:handle_wind(player, dt)
    -- Wind physics will go here. For now, it does nothing.
end

function Level:check_hazard_collision(player)
    for _, h_rect in ipairs(self.hazards) do
        local hazard = {
            x = h_rect[1], y = h_rect[2],
            width = h_rect[3], height = h_rect[4]
        }
        if check_collision(player, hazard) then
            return true
        end
    end
    return false
end

function Level:check_gate_collision(player)
    return check_collision(player, self.gate)
end

function Level:reset()
    if g_particles then
        g_particles.emitters = {}
    end
end

function Level:handle_collisions(player)
    player.is_grounded = false
    player.on_wall = 0 -- -1 for left wall, 1 for right wall, 0 for none

    for _, p_rect in ipairs(self.platforms) do
        local platform = {
            x = p_rect[1], y = p_rect[2],
            width = p_rect[3], height = p_rect[4]
        }
        
        if check_collision(player, platform) then
            -- Get center points
            local player_center_x = player.x + player.width / 2
            local player_center_y = player.y + player.height / 2
            local platform_center_x = platform.x + platform.width / 2
            local platform_center_y = platform.y + platform.height / 2
            
            -- Calculate overlap
            local overlap_x = (player.width / 2 + platform.width / 2) - math.abs(player_center_x - platform_center_x)
            local overlap_y = (player.height / 2 + platform.height / 2) - math.abs(player_center_y - platform_center_y)

            if overlap_x < overlap_y then
                -- Horizontal collision
                if player_center_x < platform_center_x then -- Player is to the left
                    player.x = platform.x - player.width
                    player.on_wall = -1
                else -- Player is to the right
                    player.x = platform.x + platform.width
                    player.on_wall = 1
                end
                if player.is_dashing then player.is_dashing = false; player.vx = 0 end
            else
                -- Vertical collision
                if player_center_y < platform_center_y then -- Player is above
                    local was_airborne = not player.is_grounded
                    player.y = platform.y - player.height
                    if player.vy >= 0 then
                        if was_airborne then
                            g_effects:land(player.x + player.width/2, player.y + player.height)
                        end
                        player.vy = 0
                        player.is_grounded = true
                        player.jumps_made = 0
                        if player.is_dashing then player.is_dashing = false; player.vy = 0 end
                    end
                else -- Player is below
                    player.y = platform.y + platform.height
                    player.vy = 0
                end
            end
        end
    end
end

return Level