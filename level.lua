Level = {}
Level.__index = Level

local TILE_SIZE = 32

function Level:new()
    local self = setmetatable({}, Level)
    
    self.tileset = love.graphics.newImage("tiles.png")
    self.quads = {}
    local ts_width = self.tileset:getWidth()
    for x = 0, ts_width - TILE_SIZE, TILE_SIZE do
        table.insert(self.quads, love.graphics.newQuad(x, 0, TILE_SIZE, TILE_SIZE, ts_width, self.tileset:getHeight()))
    end

    self.map = {}
    self.map_width = 0
    self.map_height = 0

    if g_current_circle == 0 then
        self.start_pos = {x = 100, y = 400}
        self.gate = {x = 750, y = 430, width = TILE_SIZE, height = TILE_SIZE * 2}
        local map_data = {
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "   111111           ",
            "                    ",
            "            111111  ",
            "                    ",
            "11111111111111111111",
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
            if tile_id > 0 and self.quads[tile_id] then
                love.graphics.draw(self.tileset, self.quads[tile_id], (x-1) * TILE_SIZE, (y-1) * TILE_SIZE)
            end
        end
    end
    
    -- Draw gate
    love.graphics.setColor(1, 0, 1)
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

function Level:handle_collisions(player)
    -- This will need a new tile-based collision system.
    -- For now, we'll just implement a simple floor.
    if player.y + player.height > self.map_height * TILE_SIZE - TILE_SIZE then
        player.y = self.map_height * TILE_SIZE - TILE_SIZE - player.height
        player.vy = 0
        player.is_grounded = true
        player.jumps_made = 0
    end
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

return Level