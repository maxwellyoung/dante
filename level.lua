Level = {}
Level.__index = Level

local TILE_SIZE = 32

local circle_data = {
    -- Paradiso
    { bg = {0.8, 0.9, 1}, gate_color = {0.2, 0.2, 0.8}, locked_gate_color = {0.8, 0.2, 0.2} },
    -- Limbo
    { bg = {0.3, 0.3, 0.4}, gate_color = {0.8, 0.8, 0.2}, locked_gate_color = {0.8, 0.2, 0.2} },
    -- Lust
    { bg = {0.6, 0.2, 0.4}, gate_color = {0.2, 0.8, 0.2}, locked_gate_color = {0.8, 0.2, 0.2} },
    -- Gluttony
    { bg = {0.3, 0.5, 0.2}, gate_color = {0.8, 0.2, 0.2}, locked_gate_color = {0.8, 0.2, 0.2} },
    -- Greed
    { bg = {0.7, 0.6, 0.1}, gate_color = {0.2, 0.2, 0.8}, locked_gate_color = {0.8, 0.2, 0.2} },
}

function Level:new(map_data)
    local self = setmetatable({}, Level)
    self.npcs = {}
    self.crumbling_blocks = {}
    self.moving_platforms = {}
    self.timed_traps = {}
    self.pullable_blocks = {}
    self.momentum_blocks = {}
    self.refraction_zones = {}
    
    local current_circle_info = circle_data[g_current_circle + 1] or {}
    self.fragments_required = current_circle_info.fragments_required or 1
    self.background_color = current_circle_info.bg or {0.1, 0.1, 0.1}
    self.gate_color = current_circle_info.gate_color or {1,0,1}
    self.locked_gate_color = current_circle_info.locked_gate_color or {1,0,0}

    -- Try to load tileset, but handle missing/broken tiles gracefully
    self.tileset = nil
    self.quads = {}
    local ok, img = pcall(love.graphics.newImage, "tiles.png")
    if ok and img:getWidth() > 1 and img:getHeight() > 1 then
        self.tileset = img
        self.tileset:setFilter("nearest", "nearest")
        local tile_size = 16
        for y=0, self.tileset:getHeight() - tile_size, tile_size do
            for x=0, self.tileset:getWidth() - tile_size, tile_size do
                table.insert(self.quads, love.graphics.newQuad(x, y, tile_size, tile_size, self.tileset:getDimensions()))
            end
        end
    end

    self.map = {}
    self.map_width = 0
    self.map_height = 0

    self.geysers = {}
    self.wind_areas = {}
    self.hazards = {}
    
    self:load_map(map_data)
    
    return self
end

function Level:load_map(map_data)
    self.map_height = #map_data
    self.map_width = #map_data[1]
    g_collectibles:clear()
    g_enemies:clear()

    -- Reset dynamic properties
    self.start_pos = { x=100, y=100 }
    self.gate = { x=200, y=100, width=TILE_SIZE, height=TILE_SIZE*2 }
    self.npcs = {}
    self.moving_platforms = {}
    self.timed_traps = {}
    self.pullable_blocks = {}
    self.momentum_blocks = {}

    local moving_platform_defs = {}
    local pull_block_id_counter = 1

    for y, row in ipairs(map_data) do
        self.map[y] = {}
        for x = 1, #row do
            local char = string.sub(row, x, x)
            local tile_id = tonumber(char) or 0
            
            self.map[y][x] = 0 -- Default to air
            if char == '1' then self.map[y][x] = 1
            elseif char == '2' then self.map[y][x] = 2
            elseif char == '3' then self.map[y][x] = 3
            elseif char == '4' then
                self.map[y][x] = 4
                self.crumbling_blocks[tostring(x) .. "," .. tostring(y)] = {
                    x = x, y = y, state = 'solid', timer = 0
                }
            elseif char == 'C' then g_collectibles:add((x - 1) * TILE_SIZE + 8, (y - 1) * TILE_SIZE + 8)
            elseif char == 'E' then g_enemies:spawn((x - 1) * TILE_SIZE, (y - 1) * TILE_SIZE, 'walker')
            elseif char == 'H' then g_enemies:spawn((x - 1) * TILE_SIZE, (y - 1) * TILE_SIZE, 'harpy')
            elseif char == 'P' then self.start_pos = { x = (x-1) * TILE_SIZE, y = (y-1) * TILE_SIZE }
            elseif char == 'V' then table.insert(self.npcs, { x = (x-1)*TILE_SIZE, y = (y-1)*TILE_SIZE, width=TILE_SIZE, height=TILE_SIZE })
            elseif char == 'G' then
                self.gate = { x = (x-1) * TILE_SIZE, y = (y-1) * TILE_SIZE, width = TILE_SIZE, height = TILE_SIZE }
                if g_current_circle == 0 then self.gate.height = TILE_SIZE * 2 end
            elseif char == 'M' then
                local id = tostring(x)..","..tostring(y)
                moving_platform_defs[id] = {start_x = (x-1)*TILE_SIZE, start_y = (y-1)*TILE_SIZE, end_x = (x-1)*TILE_SIZE, end_y = (y-1)*TILE_SIZE}
            elseif char == 'T' then
                table.insert(self.timed_traps, self:create_timed_trap(x, y))
            elseif char == 'R' then
                table.insert(self.refraction_zones, {
                    x = (x - 1) * TILE_SIZE, y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE, height = TILE_SIZE
                })
            elseif char == 'B' then
                table.insert(self.momentum_blocks, {
                    id = "B" .. pull_block_id_counter,
                    x = (x - 1) * TILE_SIZE, y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE, height = TILE_SIZE,
                    vx = 0, vy = 0,
                    is_hooked = false
                })
                pull_block_id_counter = pull_block_id_counter + 1
            elseif char == '5' then
                table.insert(self.pullable_blocks, {
                    id = pull_block_id_counter,
                    x = (x - 1) * TILE_SIZE, y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE, height = TILE_SIZE,
                    vx = 0, vy = 0,
                    is_hooked = false
                })
                self.map[y][x] = 0 -- Leave an empty space where the block was
                pull_block_id_counter = pull_block_id_counter + 1
            elseif char == '-' or char == '|' then
                -- Path character, handled below
            end
        end
    end
    
    -- Process moving platform paths
    for id, def in pairs(moving_platform_defs) do
        local x, y = tonumber(id:match("^(%d+),"):sub(1)), tonumber(id:match(",(%d+)$"):sub(1))
        -- Horizontal path
        local path_char = map_data[y]:sub(x+1, x+1)
        if path_char == '-' then
            local end_x = x
            while map_data[y]:sub(end_x+1, end_x+1) == '-' do end_x = end_x + 1 end
            if map_data[y]:sub(end_x+1, end_x+1) == 'M' then
                def.end_x = end_x * TILE_SIZE
                table.insert(self.moving_platforms, self:create_moving_platform(def))
            end
        end
    end

    -- Assign dialogue based on circle
    if g_current_circle == 0 then
        for i, npc in ipairs(self.npcs) do npc.dialogue = "Welcome, my son. The Labyrinth is a cruel puzzle, but I have given you the tools to solve it. Climb. The walls are your allies." end
    elseif g_current_circle == 1 then
        for i, npc in ipairs(self.npcs) do npc.dialogue = "Ariadne's Thread will guide you where your feet cannot. Aim true." end
    elseif g_current_circle == 2 then
        for i, npc in ipairs(self.npcs) do npc.dialogue = "Here, a gift of wax and feather. They will not carry you to Olympus, but they will aid your ascent. Do not fly too close to the sun... or the ceiling." end
    elseif g_current_circle == 3 then
        for i, npc in ipairs(self.npcs) do npc.dialogue = "Some things, once set in motion, do not easily stop." end
    else
        for i, npc in ipairs(self.npcs) do npc.dialogue = "More challenges lie ahead." end
    end
end

function Level:create_timed_trap(x, y)
    -- Randomize the initial state to avoid all traps syncing up
    local start_state = math.random(1, 2) == 1 and 'on' or 'off'
    return {
        grid_x = x, grid_y = y,
        width = TILE_SIZE, height = TILE_SIZE,
        state = start_state, -- on, off, warning
        timer = math.random() * 2, -- Time until next state change
        on_duration = 1.5,
        off_duration = 2.0,
        warning_duration = 0.5,
    }
end

function Level:create_moving_platform(def)
    return {
        x = def.start_x, y = def.start_y,
        width = TILE_SIZE, height = TILE_SIZE/2,
        start_x = def.start_x, start_y = def.start_y,
        end_x = def.end_x, end_y = def.end_y,
        speed = 50,
        direction = 1, -- 1 for forward, -1 for backward
        dx = 0 -- The delta_x for this frame
    }
end

function Level:draw()
    -- Calculate visible tile range (with buffer for smooth scrolling)
    local start_x = math.max(1, math.floor(g_camera.x / TILE_SIZE))
    local end_x = math.min(self.map_width, math.ceil((g_camera.x + g_native_width) / TILE_SIZE) + 1)
    local start_y = math.max(1, math.floor(g_camera.y / TILE_SIZE))
    local end_y = math.min(self.map_height, math.ceil((g_camera.y + g_native_height) / TILE_SIZE) + 1)

    -- Draw solid tiles with the new shader (only visible tiles)
    love.graphics.setShader(g_tile_shader)
    for y = start_y, end_y do
        for x = start_x, end_x do
            local tile_id = self.map[y] and self.map[y][x]
            if tile_id == 1 then
                local x_pos, y_pos = (x-1) * TILE_SIZE, (y-1) * TILE_SIZE
                local screen_x, screen_y = g_camera:to_screen(x_pos, y_pos)
                g_tile_shader:send("base_color", {0.4, 0.45, 0.55})
                g_tile_shader:send("tile_pos", {screen_x, screen_y})
                love.graphics.rectangle("fill", x_pos, y_pos, TILE_SIZE, TILE_SIZE)
            end
        end
    end
    love.graphics.setShader()

    -- Draw other tile types (only visible tiles)
    for y = start_y, end_y do
        for x = start_x, end_x do
            local tile_id = self.map[y] and self.map[y][x]
            if tile_id and tile_id > 1 then
                local x_pos, y_pos = (x-1) * TILE_SIZE, (y-1) * TILE_SIZE
                -- Use tileset if available, otherwise colored rectangles
                if self.tileset and self.quads[tile_id] then
                    love.graphics.draw(self.tileset, self.quads[tile_id], x_pos, y_pos, 0, TILE_SIZE/16, TILE_SIZE/16)
                else
                    -- Fallback colors for different tile types
                    if tile_id == 2 then
                        love.graphics.setColor(0.8, 0.2, 0.2) -- Hazard (red)
                    elseif tile_id == 3 then
                        love.graphics.setColor(0.3, 0.5, 0.3) -- Sludge (green)
                    elseif tile_id == 4 then
                        love.graphics.setColor(0.6, 0.5, 0.4) -- Crumbling (brown)
                    else
                        love.graphics.setColor(0.5, 0.5, 0.6) -- Default
                    end
                    love.graphics.rectangle("fill", x_pos, y_pos, TILE_SIZE, TILE_SIZE)
                end
            end
        end
    end
    love.graphics.setColor(1, 1, 1)
    
    -- Draw timed traps
    for _, trap in ipairs(self.timed_traps) do
        if trap.state == 'on' then
            love.graphics.setColor(0.5, 0.1, 0.1)
            love.graphics.rectangle("fill", (trap.grid_x-1) * TILE_SIZE, (trap.grid_y-1) * TILE_SIZE, trap.width, trap.height)
        elseif trap.state == 'warning' then
            local pulse = (math.sin(love.timer.getTime() * 20) + 1) / 2
            love.graphics.setColor(0.9, 0.9, 0.2, 0.5 + pulse * 0.3)
            love.graphics.rectangle("fill", (trap.grid_x-1) * TILE_SIZE, (trap.grid_y-1) * TILE_SIZE, trap.width, trap.height)
        end
    end

    -- Draw moving platforms
    love.graphics.setColor(0.4, 0.4, 0.5)
    for _, p in ipairs(self.moving_platforms) do
        love.graphics.rectangle("fill", p.x, p.y + p.height/2, p.width, p.height)
    end
    
    -- Draw pullable blocks
    love.graphics.setColor(0.6, 0.5, 0.4)
    for _, block in ipairs(self.pullable_blocks) do
        love.graphics.rectangle("fill", block.x, block.y, block.width, block.height)
    end
    
    -- Draw momentum blocks
    love.graphics.setColor(0.4, 0.6, 1.0) -- Distinct color for momentum blocks
    for _, block in ipairs(self.momentum_blocks) do
        love.graphics.rectangle("fill", block.x, block.y, block.width, block.height)
    end

    -- Draw gate
    if g_player.fragments >= self.fragments_required then
        love.graphics.setColor(self.gate_color[1], self.gate_color[2], self.gate_color[3])
    else
        love.graphics.setColor(self.locked_gate_color[1], self.locked_gate_color[2], self.locked_gate_color[3])
    end
    love.graphics.rectangle("fill", self.gate.x, self.gate.y, self.gate.width, self.gate.height)
    
    -- Draw NPCs
    love.graphics.setColor(0.8, 0.8, 1) -- A ghostly blue for Virgil
    for _, npc in ipairs(self.npcs) do
        love.graphics.rectangle("fill", npc.x, npc.y, npc.width, npc.height)
    end
    
    love.graphics.setColor(1, 1, 1)
end

function Level:get_tile(px, py)
    if px < 0 or py < 0 or px >= self.map_width * TILE_SIZE or py >= self.map_height * TILE_SIZE then
        return 1 -- Treat out-of-bounds as solid
    end
    local tile_x = math.floor(px / TILE_SIZE) + 1
    local tile_y = math.floor(py / TILE_SIZE) + 1
    return self.map[tile_y] and self.map[tile_y][tile_x] or 0
end

function Level:is_solid_at_pixel(px, py)
    local tile_id = self:get_tile(px, py)
    return tile_id == 1 or tile_id == 2
end

function Level:handle_player_collision(player)
    player.is_grounded = false
    local on_wall = 0
    local is_slowed = false

    -- Horizontal collision
    player.x = player.x + player.vx * love.timer.getDelta()
    local margin = 1 -- Add a small margin to prevent getting stuck in walls
    if player.vx > 0 then
        if (self:get_tile(player.x + player.width, player.y + margin) > 0 and self:get_tile(player.x + player.width, player.y + player.height - margin) > 0) or
           (self:get_tile(player.x + player.width, player.y + player.height/2) > 0) then
            player.x = math.floor(player.x / TILE_SIZE) * TILE_SIZE
            player.vx = 0
        end
    elseif player.vx < 0 then
        if (self:get_tile(player.x, player.y + margin) > 0 and self:get_tile(player.x, player.y + player.height - margin) > 0) or
           (self:get_tile(player.x, player.y + player.height/2) > 0) then
            player.x = math.floor((player.x + TILE_SIZE) / TILE_SIZE) * TILE_SIZE
            player.vx = 0
        end
    end

    -- Vertical collision
    player.y = player.y + player.vy * love.timer.getDelta()
    local on_moving_platform = false
    local platform_dx = 0

    -- Check moving platforms first
    for _, p in ipairs(self.moving_platforms) do
        if player.vy >= 0 and
           player.x + player.width > p.x and
           player.x < p.x + p.width and
           player.y + player.height >= p.y + p.height/2 and
           player.y + player.height <= p.y + p.height/2 + 10 then -- 10px landing tolerance
            
            player.y = p.y + p.height/2 - player.height
            player.vy = 0
            player.is_grounded = true
            player.jumps_made = 0
            on_moving_platform = true
            platform_dx = p.dx
            break
        end
    end
    
    if not on_moving_platform then
        if player.vy > 0 then
            if (self:get_tile(player.x + margin, player.y + player.height) > 0) or (self:get_tile(player.x + player.width - margin, player.y + player.height) > 0) then
                player.y = math.floor((player.y + player.height) / TILE_SIZE) * TILE_SIZE - player.height
                player.vy = 0
                player.is_grounded = true
                player.jumps_made = 0
            end
        elseif player.vy < 0 then
            if (self:get_tile(player.x + margin, player.y) > 0) or (self:get_tile(player.x + player.width - margin, player.y) > 0) then
                player.y = math.floor((player.y + TILE_SIZE) / TILE_SIZE) * TILE_SIZE
                player.vy = 0
            end
        end
    end
    
    player.x = player.x + platform_dx

    -- Check for wall contact for wall sliding
    if self:get_tile(player.x - 1, player.y + player.height/2) > 0 then on_wall = -1 end
    if self:get_tile(player.x + player.width + 1, player.y + player.height/2) > 0 then on_wall = 1 end

    -- Check for sludge
    if self:get_tile(player.x + player.width/2, player.y + player.height + 1) == 3 then
        is_slowed = true
    end

    player:set_collision_context({on_wall = on_wall})
end

function Level:check_gate_collision(player)
    if not self.gate then return false end
    return player.x < self.gate.x + self.gate.width and
           player.x + player.width > self.gate.x and
           player.y < self.gate.y + self.gate.height and
           player.y + player.height > self.gate.y
end

function Level:handle_hazards_collision(player)
    -- Siren Blocks
    if self:get_tile(player.x + player.width/2, player.y + player.height/2) == 2 then
        return true
    end
    return false
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

    -- Update timed traps
    for _, trap in ipairs(self.timed_traps) do
        trap.timer = trap.timer - dt
        if trap.timer <= 0 then
            if trap.state == 'off' then
                trap.state = 'warning'
                trap.timer = trap.warning_duration
            elseif trap.state == 'warning' then
                trap.state = 'on'
                trap.timer = trap.on_duration
            elseif trap.state == 'on' then
                trap.state = 'off'
                trap.timer = trap.off_duration
            end
        end
    end

    -- Update moving platforms
    for _, p in ipairs(self.moving_platforms) do
        local old_x = p.x
        if p.direction == 1 then
            p.x = p.x + p.speed * dt
            if p.x >= p.end_x then
                p.x = p.end_x
                p.direction = -1
            end
        else -- direction == -1
            p.x = p.x - p.speed * dt
            if p.x <= p.start_x then
                p.x = p.start_x
                p.direction = 1
            end
        end
        p.dx = p.x - old_x
    end

    -- Update crumbling blocks
    for key, block in pairs(self.crumbling_blocks) do
        if block.state == 'crumbling' then
            block.timer = block.timer - dt
            if block.timer <= 0 then
                self.map[block.y][block.x] = 0 -- Make it air
                self.crumbling_blocks[key] = nil -- Remove it
            end
        end
    end

    -- Update pullable block physics
    for _, block in ipairs(self.pullable_blocks) do
        if not block.is_hooked then
            -- Apply gravity only when not being pulled
            block.vy = block.vy + 1800 * dt
        end
        
        -- Apply velocity
        block.x = block.x + block.vx * dt
        block.y = block.y + block.vy * dt
        
        -- Dampen velocity (friction)
        block.vx = block.vx * 0.9
        
        -- Collision with solid tiles
        local collided = false
        if self:get_tile(block.x + block.width/2, block.y + block.height) > 0 then
            if math.abs(block.vx) > 5 then
                g_effects:rumble(block.x + block.width/2, block.y + block.height)
            end
            block.y = math.floor((block.y + block.height) / TILE_SIZE) * TILE_SIZE - block.height
            if block.vy > 150 then -- Only shake on hard vertical impacts
                collided = true
            end
            block.vy = 0
            block.vx = block.vx * 0.8 -- More friction on ground
        end

        -- Horizontal collision check for screen shake
        if self:get_tile(block.x - 1, block.y + block.height/2) > 0 or self:get_tile(block.x + block.width + 1, block.y + block.height/2) > 0 then
            if math.abs(block.vx) > 150 then -- Only shake on hard horizontal impacts
                collided = true
            end
            block.vx = 0
        end

        if collided then
            g_camera:shake(0.1, 2) -- Reduced intensity
        end
    end

    -- Update momentum block physics
    for _, block in ipairs(self.momentum_blocks) do
        if not block.is_hooked then
            block.vy = block.vy + 1800 * dt
        end
        
        block.x = block.x + block.vx * dt
        block.y = block.y + block.vy * dt
        
        -- No velocity damping for momentum blocks
        
        -- Collision with solid tiles
        if self:get_tile(block.x + block.width/2, block.y + block.height) > 0 then
            block.y = math.floor((block.y + block.height) / TILE_SIZE) * TILE_SIZE - block.height
            block.vy = 0
        end

        if self:get_tile(block.x - 1, block.y + block.height/2) > 0 or self:get_tile(block.x + block.width + 1, block.y + block.height/2) > 0 then
            block.vx = 0
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
    
    -- Check for lethal tiles at player corners
    local left_tile = math.floor(player.x / TILE_SIZE) + 1
    local right_tile = math.floor((player.x + player.width) / TILE_SIZE) + 1
    local top_tile = math.floor(player.y / TILE_SIZE) + 1
    local bottom_tile = math.floor((player.y + player.height) / TILE_SIZE) + 1
    
    local tiles_to_check = {{left_tile, top_tile}, {right_tile, top_tile}, {left_tile, bottom_tile}, {right_tile, bottom_tile}}
    for _, pos in ipairs(tiles_to_check) do
        local tx, ty = pos[1], pos[2]
        if self.map[ty] and self.map[ty][tx] and self.map[ty][tx] == 2 then
            return true
        end
    end

    return false
end

function Level:handle_tile_effects(player)
    player.is_slowed = false
    local player_center_x = player.x + player.width / 2
    local player_feet_y = player.y + player.height
    
    local tile_id = self:get_tile(player_center_x, player_feet_y)

    if tile_id == 3 then
        player.is_slowed = true
    end
end

function Level:reset()
    if g_particles then g_particles:clear() end
end

function Level:set_tile(world_x, world_y, tile_id)
    local tile_x = math.floor(world_x / TILE_SIZE) + 1
    local tile_y = math.floor(world_y / TILE_SIZE) + 1

    if tile_y > 0 and tile_y <= self.map_height and tile_x > 0 and tile_x <= self.map_width then
        self.map[tile_y][tile_x] = tile_id
    end
end

function Level:print_map_data()
    print("--- New Level Data ---")
    print("{")
    for y = 1, self.map_height do
        local row_str = "    \""
        for x = 1, self.map_width do
            local tile = self.map[y][x]
            if tile == 0 then
                row_str = row_str .. " "
            else
                row_str = row_str .. tostring(tile)
            end
        end
        row_str = row_str .. "\","
        print(row_str)
    end
    print("}")
    print("--- End Level Data ---")
end

function Level:get_pullable_block_at(world_x, world_y)
    for _, block in ipairs(self.pullable_blocks) do
        if world_x >= block.x and world_x < block.x + block.width and
           world_y >= block.y and world_y < block.y + block.height then
            return block
        end
    end
    return nil
end

return Level