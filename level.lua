Level = {}
Level.__index = Level

local TILE_SIZE = 32

local circle_data = {
    -- Paradiso
    { bg = {0.8, 0.9, 1}, gate_color = {0.2, 0.2, 0.8} },
    -- Limbo
    { bg = {0.3, 0.3, 0.4}, gate_color = {0.8, 0.8, 0.2} },
    -- Lust
    { bg = {0.6, 0.2, 0.4}, gate_color = {0.2, 0.8, 0.2} },
    -- Gluttony
    { bg = {0.3, 0.5, 0.2}, gate_color = {0.8, 0.2, 0.2} },
    -- Greed
    { bg = {0.7, 0.6, 0.1}, gate_color = {0.2, 0.2, 0.8} },
    -- TODO: Add colors for other circles
}

function Level:new()
    local self = setmetatable({}, Level)
    
    local current_circle_data = circle_data[g_current_circle + 1] or {bg = {0.1, 0.1, 0.1}, gate_color = {1,0,1}}
    self.background_color = current_circle_data.bg
    self.gate_color = current_circle_data.gate_color

    self.map = {}
    self.map_width = 0
    self.map_height = 0

    self.geysers = {}
    self.wind_areas = {}
    self.hazards = {}

    if g_current_circle == 0 then
        self.start_pos = {x = 100, y = 880}
        self.gate = {x = 720, y = 110, width = TILE_SIZE, height = TILE_SIZE * 2}
        local map_data = {
            "1                    1",
            "1                    1",
            "1         GG         1",
            "1                    1",
            "1                    1",
            "1   11111            1",
            "1                    1",
            "1            111     1",
            "1                    1",
            "1     111            1",
            "1                    1",
            "1                    1",
            "1  11                1",
            "1                    1",
            "1           1111     1",
            "1                    1",
            "1      11            1",
            "1                    1",
            "1                    1",
            "1   111              1",
            "1                    1",
            "1               111  1",
            "1                    1",
            "1   111111           1",
            "1                    1",
            "1                    1",
            "1                    1",
            "1                    1",
            "1111111111111111111111",
        }
        self:load_map(map_data)
    else
        -- Fallback for other circles for now
        self.start_pos = {x = 100, y = 400}
        self.gate = {x = 750, y = 430, width = TILE_SIZE, height = TILE_SIZE * 2}
        local map_data = { "11111111111111111111" }
        self:load_map(map_data)
    end
    
    return self
end

function Level:load_map(map_data)
    self.map_height = #map_data
    self.map_width = #map_data[1]
    for y, row in ipairs(map_data) do
        self.map[y] = {}
        for x = 1, #row do
            self.map[y][x] = tonumber(string.sub(row, x, x)) or 0
        end
    end
end

function Level:draw()
    for y = 1, self.map_height do
        for x = 1, self.map_width do
            local tile_id = self.map[y][x]
            if tile_id > 0 then
                love.graphics.setColor(0.2, 0.2, 0.25) -- Tile fill
                love.graphics.rectangle("fill", (x-1) * TILE_SIZE, (y-1) * TILE_SIZE, TILE_SIZE, TILE_SIZE)
                love.graphics.setColor(self.background_color) -- Outline color matching background
                love.graphics.rectangle("line", (x-1) * TILE_SIZE, (y-1) * TILE_SIZE, TILE_SIZE, TILE_SIZE)
            end
        end
    end
    
    -- Draw gate
    love.graphics.setColor(self.gate_color[1], self.gate_color[2], self.gate_color[3])
    love.graphics.rectangle("fill", self.gate.x, self.gate.y, self.gate.width, self.gate.height)
    love.graphics.setColor(1, 1, 1)
end

function Level:get_tile(world_x, world_y)
    if world_x < 0 or world_y < 0 then return 0 end
    local tile_x = math.floor(world_x / TILE_SIZE) + 1
    local tile_y = math.floor(world_y / TILE_SIZE) + 1
    if self.map[tile_y] and self.map[tile_y][tile_x] then
        return self.map[tile_y][tile_x]
    end
    return 0
end

function Level:handle_collisions_x(player)
    player.on_wall = 0

    if player.vx == 0 then return end
    
    local left_tile = math.floor(player.x / TILE_SIZE) + 1
    local right_tile = math.floor((player.x + player.width) / TILE_SIZE) + 1
    local top_tile = math.floor((player.y + 1) / TILE_SIZE) + 1
    local bottom_tile = math.floor((player.y + player.height - 1) / TILE_SIZE) + 1
    
    if player.vx > 0 then -- Moving right
        for y = top_tile, bottom_tile do
            if self.map[y] and self.map[y][right_tile] and self.map[y][right_tile] > 0 then
                player.x = (right_tile - 1) * TILE_SIZE - player.width
                player.vx = 0
                player.on_wall = 1
                return
            end
        end
    else -- Moving left
        for y = top_tile, bottom_tile do
            if self.map[y] and self.map[y][left_tile] and self.map[y][left_tile] > 0 then
                player.x = left_tile * TILE_SIZE
                player.vx = 0
                player.on_wall = -1
                return
            end
        end
    end
end

function Level:handle_collisions_y(player)
    local was_grounded = player.is_grounded

    local left_tile = math.floor((player.x + 1) / TILE_SIZE) + 1
    local right_tile = math.floor((player.x + player.width - 1) / TILE_SIZE) + 1
    local top_tile = math.floor(player.y / TILE_SIZE) + 1
    local bottom_tile = math.floor((player.y + player.height) / TILE_SIZE) + 1

    if player.vy >= 0 then -- Moving down (or stationary)
        local ground_contact = false
        for x = left_tile, right_tile do
            if self.map[bottom_tile] and self.map[bottom_tile][x] and self.map[bottom_tile][x] > 0 then
                if not was_grounded then
                    g_effects:land(player.x + player.width / 2, player.y + player.height)
                end
                player.y = (bottom_tile - 1) * TILE_SIZE - player.height
                player.vy = 0
                player.is_grounded = true
                player.jumps_made = 0
                ground_contact = true
                break
            end
        end
        if not ground_contact then
            player.is_grounded = false
        end
    else -- Moving up
        player.is_grounded = false
        for x = left_tile, right_tile do
            if self.map[top_tile] and self.map[top_tile][x] and self.map[top_tile][x] > 0 then
                player.y = top_tile * TILE_SIZE
                player.vy = 0
                return
            end
        end
    end
end

function Level:handle_collisions(player, dt)
    self:handle_collisions_x(player)
    self:handle_collisions_y(player)
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
    if g_particles then g_particles:clear() end
end

return Level