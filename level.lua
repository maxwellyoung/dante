Level = {}
Level.__index = Level

local SceneContract = require("scene_contract")

local TILE_SIZE = 32

local function distance_sq(x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    return dx * dx + dy * dy
end

local function get_mark_meaningful_action(self)
    return self.services and self.services.mark_meaningful_action or g_mark_meaningful_action
end

local function get_record_environment_label(self)
    return self.services and self.services.record_environment_label or g_record_environment_label
end

local function get_record_checkpoint_activated(self)
    return self.services and self.services.record_checkpoint_activated or rawget(_G, "g_record_checkpoint_activated")
end

function Level:new(scene, services)
    local class = self or Level
    local instance = setmetatable({}, class)
    instance.services = services
    instance.npcs = {}
    instance.crumbling_blocks = {}
    instance.moving_platforms = {}
    instance.timed_traps = {}
    instance.pullable_blocks = {}
    instance.momentum_blocks = {}
    instance.refraction_zones = {}
    instance.geysers = {}
    instance.hazards = {}
    instance.checkpoints = {}
    instance.grapple_anchors = {}
    instance.active_checkpoint = nil
    instance.scene = SceneContract.normalize(scene)
    instance.scene_title = instance.scene.title or instance.scene.name or "SCENE"
    instance.scene_subtitle = instance.scene.subtitle or ""
    instance.objective_text = instance.scene.objective_text or ""
    instance.hud_objective_text = instance.scene.hud_objective_text or instance.objective_text
    instance.mode = instance.scene.mode or "custom"
    instance.room_type = instance.scene.room_type or "custom"
    instance.checkpoint_id = instance.scene.checkpoint_id or "start"
    instance.encounter_id = instance.scene.encounter_id
    instance.music_state = instance.scene.music_state or "default"
    instance.fragments_required = instance.scene.fragments_required or 0
    instance.background_color = instance.scene.bg
    instance.solid_color = instance.scene.solid_color
    instance.accent_color = instance.scene.accent_color
    instance.hazard_color = instance.scene.hazard_color
    instance.sludge_color = instance.scene.sludge_color
    instance.gate_color = instance.scene.gate_color
    instance.locked_gate_color = instance.scene.locked_gate_color
    instance.show_gate = instance.scene.show_gate ~= false
    instance.removed_ability = instance.scene.removed_ability or "none"
    instance.environment_hook = instance.scene.environment_hook or ""
    instance.transition_beat = instance.scene.transition_beat or ""
    instance.weapon_profile = instance.scene.weapon_profile or {}
    instance.abilities = instance.scene.abilities or {}
    instance.wind_areas = instance.scene.wind_areas or {}
    instance.reach_zone = instance.scene.reach_zone
    instance.qa_expectations = instance.scene.qa_expectations or {}
    instance.npc_guidance = instance.scene.npc_guidance or {}
    instance.map = {}
    instance.map_width = 0
    instance.map_height = 0
    instance:load_map(instance.scene.map or {})
    return instance
end

function Level:load_map(map_data)
    self.map_height = #map_data
    self.map_width = #map_data[1]
    local collectibles = self.services and self.services.collectibles or g_collectibles
    local enemies = self.services and self.services.enemies or g_enemies
    collectibles:clear()
    enemies:clear()

    self.start_pos = { x = 100, y = 100 }
    self.gate = nil
    self.npcs = {}
    self.moving_platforms = {}
    self.timed_traps = {}
    self.pullable_blocks = {}
    self.momentum_blocks = {}
    self.crumbling_blocks = {}
    self.checkpoints = {}
    self.grapple_anchors = {}
    self.active_checkpoint = nil

    local moving_platform_defs = {}
    local pull_block_id_counter = 1

    for y, row in ipairs(map_data) do
        self.map[y] = {}
        for x = 1, #row do
            local char = string.sub(row, x, x)
            self.map[y][x] = 0

            if char == "1" then
                self.map[y][x] = 1
            elseif char == "2" then
                self.map[y][x] = 2
            elseif char == "3" then
                self.map[y][x] = 3
            elseif char == "4" then
                self.map[y][x] = 4
                self.crumbling_blocks[tostring(x) .. "," .. tostring(y)] = {
                    x = x,
                    y = y,
                    state = "solid",
                    timer = 0,
                }
            elseif char == "C" then
                collectibles:add((x - 1) * TILE_SIZE + 8, (y - 1) * TILE_SIZE + 8)
            elseif char == "E" then
                enemies:spawn((x - 1) * TILE_SIZE, (y - 1) * TILE_SIZE, "walker")
            elseif char == "H" then
                enemies:spawn((x - 1) * TILE_SIZE, (y - 1) * TILE_SIZE, "harpy")
            elseif char == "P" then
                self.start_pos = { x = (x - 1) * TILE_SIZE, y = (y - 1) * TILE_SIZE }
            elseif char == "V" then
                local guidance = self.npc_guidance[#self.npcs + 1] or {}
                table.insert(self.npcs, {
                    x = (x - 1) * TILE_SIZE,
                    y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE,
                    height = TILE_SIZE,
                    speaker = guidance.speaker or "Virgil",
                    line = guidance.line,
                    trigger_radius = guidance.trigger_radius or 60,
                    triggered = false,
                    active_hint = false,
                })
            elseif char == "G" then
                self.gate = {
                    x = (x - 1) * TILE_SIZE,
                    y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE,
                    height = TILE_SIZE,
                }
            elseif char == "K" then
                table.insert(self.checkpoints, {
                    id = string.format("%s-%d", self.checkpoint_id, #self.checkpoints + 1),
                    x = (x - 1) * TILE_SIZE,
                    y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE,
                    height = TILE_SIZE,
                    active = false,
                })
            elseif char == "A" then
                table.insert(self.grapple_anchors, {
                    x = (x - 1) * TILE_SIZE + TILE_SIZE / 2,
                    y = (y - 1) * TILE_SIZE + TILE_SIZE / 2,
                    radius = 10,
                })
            elseif char == "M" then
                local id = tostring(x) .. "," .. tostring(y)
                moving_platform_defs[id] = {
                    start_x = (x - 1) * TILE_SIZE,
                    start_y = (y - 1) * TILE_SIZE,
                    end_x = (x - 1) * TILE_SIZE,
                    end_y = (y - 1) * TILE_SIZE,
                }
            elseif char == "T" then
                table.insert(self.timed_traps, self:create_timed_trap(x, y))
            elseif char == "B" then
                table.insert(self.momentum_blocks, {
                    id = "B" .. pull_block_id_counter,
                    x = (x - 1) * TILE_SIZE,
                    y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE,
                    height = TILE_SIZE,
                    vx = 0,
                    vy = 0,
                    is_hooked = false,
                })
                pull_block_id_counter = pull_block_id_counter + 1
            elseif char == "5" then
                table.insert(self.pullable_blocks, {
                    id = pull_block_id_counter,
                    x = (x - 1) * TILE_SIZE,
                    y = (y - 1) * TILE_SIZE,
                    width = TILE_SIZE,
                    height = TILE_SIZE,
                    vx = 0,
                    vy = 0,
                    is_hooked = false,
                })
                pull_block_id_counter = pull_block_id_counter + 1
            end
        end
    end

    for id, def in pairs(moving_platform_defs) do
        local x = tonumber(id:match("^(%d+),"))
        local y = tonumber(id:match(",(%d+)$"))
        local path_char = map_data[y]:sub(x + 1, x + 1)
        if path_char == "-" then
            local end_x = x
            while map_data[y]:sub(end_x + 1, end_x + 1) == "-" do
                end_x = end_x + 1
            end
            if map_data[y]:sub(end_x + 1, end_x + 1) == "M" then
                def.end_x = end_x * TILE_SIZE
                table.insert(self.moving_platforms, self:create_moving_platform(def))
            end
        end
    end
end

function Level.create_timed_trap(_, x, y)
    local start_state = math.random(1, 2) == 1 and "on" or "off"
    return {
        grid_x = x,
        grid_y = y,
        width = TILE_SIZE,
        height = TILE_SIZE,
        state = start_state,
        timer = math.random() * 2,
        on_duration = 1.5,
        off_duration = 2.0,
        warning_duration = 0.5,
    }
end

function Level.create_moving_platform(_, def)
    return {
        x = def.start_x,
        y = def.start_y,
        width = TILE_SIZE,
        height = TILE_SIZE / 2,
        start_x = def.start_x,
        start_y = def.start_y,
        end_x = def.end_x,
        end_y = def.end_y,
        speed = 50,
        direction = 1,
        dx = 0,
    }
end

function Level:draw()
    local player = self.services and self.services.player or g_player
    local start_x = math.max(1, math.floor(g_camera.x / TILE_SIZE))
    local end_x = math.min(self.map_width, math.ceil((g_camera.x + g_native_width) / TILE_SIZE) + 1)
    local start_y = math.max(1, math.floor(g_camera.y / TILE_SIZE))
    local end_y =
        math.min(self.map_height, math.ceil((g_camera.y + g_native_height) / TILE_SIZE) + 1)

    for y = start_y, end_y do
        for x = start_x, end_x do
            local tile_id = self.map[y] and self.map[y][x]
            if tile_id and tile_id > 0 then
                local x_pos = (x - 1) * TILE_SIZE
                local y_pos = (y - 1) * TILE_SIZE
                if tile_id == 1 then
                    love.graphics.setColor(self.solid_color)
                elseif tile_id == 2 then
                    -- Lava: animated glow
                    local lava_pulse = 0.7 + 0.3 * math.sin(love.timer.getTime() * 2.5 + x * 0.8 + y * 0.5)
                    love.graphics.setColor(
                        self.hazard_color[1] * lava_pulse,
                        self.hazard_color[2] * lava_pulse * 0.6,
                        self.hazard_color[3] * 0.2,
                        1
                    )
                elseif tile_id == 3 then
                    love.graphics.setColor(self.sludge_color)
                elseif tile_id == 4 then
                    love.graphics.setColor(0.55, 0.48, 0.38, 1)
                else
                    love.graphics.setColor(0.45, 0.48, 0.54, 1)
                end
                love.graphics.rectangle("fill", x_pos, y_pos, TILE_SIZE, TILE_SIZE)

                if tile_id == 1 then
                    love.graphics.setColor(1, 1, 1, 0.08)
                    love.graphics.rectangle("fill", x_pos, y_pos, TILE_SIZE, 4)
                    love.graphics.setColor(0.02, 0.02, 0.03, 0.15)
                    love.graphics.rectangle("fill", x_pos, y_pos + TILE_SIZE - 4, TILE_SIZE, 4)
                elseif tile_id == 2 then
                    -- Lava surface glow
                    local glow = 0.5 + 0.5 * math.sin(love.timer.getTime() * 3 + x * 1.2)
                    love.graphics.setColor(1, 0.7, 0.1, 0.35 * glow)
                    love.graphics.rectangle("fill", x_pos, y_pos, TILE_SIZE, 5)
                    -- Bright cracks
                    love.graphics.setColor(1, 0.85, 0.3, 0.2 + 0.15 * glow)
                    love.graphics.line(x_pos + 4, y_pos + 10, x_pos + TILE_SIZE - 4, y_pos + 14)
                    love.graphics.line(x_pos + 8, y_pos + 20, x_pos + TILE_SIZE - 8, y_pos + 24)
                end
            end
        end
    end

    for _, wind in ipairs(self.wind_areas) do
        local alpha = 0.12
        local stripe_alpha = 0.22
        local stripe_step = 18
        if wind.force_x >= 0 then
            love.graphics.setColor(
                self.accent_color[1],
                self.accent_color[2],
                self.accent_color[3],
                alpha
            )
        else
            love.graphics.setColor(0.55, 0.74, 0.96, alpha)
        end
        love.graphics.rectangle("fill", wind.x, wind.y, wind.width, wind.height)
        love.graphics.setColor(1, 1, 1, stripe_alpha)
        for stripe = 0, wind.width, stripe_step do
            local start_line = wind.force_x >= 0 and stripe or stripe + 8
            love.graphics.line(
                wind.x + start_line,
                wind.y + 10,
                wind.x + start_line + (wind.force_x >= 0 and 12 or -12),
                wind.y + wind.height - 10
            )
        end
        if wind.label and wind.label ~= "" then
            love.graphics.setColor(0.95, 0.96, 1, 0.34)
            love.graphics.print(wind.label, wind.x + 8, wind.y + 6)
        end
    end

    love.graphics.setColor(0.45, 0.47, 0.53, 1)
    for _, platform in ipairs(self.moving_platforms) do
        love.graphics.rectangle(
            "fill",
            platform.x,
            platform.y + platform.height / 2,
            platform.width,
            platform.height
        )
    end

    love.graphics.setColor(0.63, 0.56, 0.44, 1)
    for _, block in ipairs(self.pullable_blocks) do
        love.graphics.rectangle("fill", block.x, block.y, block.width, block.height, 3, 3)
    end

    love.graphics.setColor(0.32, 0.58, 0.95, 1)
    for _, block in ipairs(self.momentum_blocks) do
        love.graphics.rectangle("fill", block.x, block.y, block.width, block.height, 3, 3)
    end

    for _, trap in ipairs(self.timed_traps) do
        if trap.state == "on" then
            love.graphics.setColor(self.hazard_color)
            love.graphics.rectangle(
                "fill",
                (trap.grid_x - 1) * TILE_SIZE,
                (trap.grid_y - 1) * TILE_SIZE,
                trap.width,
                trap.height
            )
        elseif trap.state == "warning" then
            local pulse = (math.sin(love.timer.getTime() * 20) + 1) / 2
            love.graphics.setColor(0.98, 0.82, 0.25, 0.45 + pulse * 0.3)
            love.graphics.rectangle(
                "fill",
                (trap.grid_x - 1) * TILE_SIZE,
                (trap.grid_y - 1) * TILE_SIZE,
                trap.width,
                trap.height
            )
        end
    end

    if self.show_gate and self.gate then
        local pulse = 0.72 + 0.18 * math.sin(love.timer.getTime() * 4)
        local gate_open = player and player.fragments >= self.fragments_required
        local gate_center_x = self.gate.x + self.gate.width / 2
        local gate_top_y = self.gate.y
        if gate_open then
            love.graphics.setColor(
                self.gate_color[1],
                self.gate_color[2],
                self.gate_color[3],
                0.22 * pulse
            )
            love.graphics.rectangle(
                "fill",
                self.gate.x - 6,
                self.gate.y - 6,
                self.gate.width + 12,
                self.gate.height + 12,
                5,
                5
            )
            love.graphics.setColor(
                self.gate_color[1],
                self.gate_color[2],
                self.gate_color[3],
                0.22 * pulse
            )
            love.graphics.rectangle(
                "fill",
                gate_center_x - 3,
                math.max(0, gate_top_y - 76),
                6,
                76
            )
            love.graphics.setColor(self.gate_color)
        else
            love.graphics.setColor(self.locked_gate_color)
        end
        love.graphics.rectangle(
            "fill",
            self.gate.x,
            self.gate.y,
            self.gate.width,
            self.gate.height,
            3,
            3
        )
        love.graphics.setColor(0.08, 0.08, 0.1, 0.65)
        love.graphics.rectangle(
            "line",
            self.gate.x + 4,
            self.gate.y + 4,
            self.gate.width - 8,
            self.gate.height - 8,
            3,
            3
        )
        if gate_open then
            love.graphics.setColor(0.98, 0.96, 0.88, 0.92)
            love.graphics.printf("EXIT", self.gate.x - 16, math.max(0, gate_top_y - 20), self.gate.width + 32, "center")
            love.graphics.setColor(self.gate_color[1], self.gate_color[2], self.gate_color[3], 0.9)
            love.graphics.polygon(
                "fill",
                gate_center_x,
                math.max(4, gate_top_y - 8),
                gate_center_x - 6,
                math.max(0, gate_top_y - 18),
                gate_center_x + 6,
                math.max(0, gate_top_y - 18)
            )
        end
    end

    for _, checkpoint in ipairs(self.checkpoints) do
        local pulse = 0.55 + 0.45 * math.sin(love.timer.getTime() * 3)
        if checkpoint.active then
            love.graphics.setColor(0.68, 0.84, 0.98, 0.18 * pulse)
            love.graphics.rectangle(
                "fill",
                checkpoint.x - 4,
                checkpoint.y - 6,
                checkpoint.width + 8,
                checkpoint.height + 12,
                4,
                4
            )
            love.graphics.setColor(0.68, 0.84, 0.98, 1)
        else
            love.graphics.setColor(0.72, 0.74, 0.78, 0.8)
        end
        love.graphics.rectangle(
            "line",
            checkpoint.x + 4,
            checkpoint.y + 4,
            checkpoint.width - 8,
            checkpoint.height - 8,
            3,
            3
        )
        love.graphics.setColor(0.96, 0.96, 0.98, checkpoint.active and 0.95 or 0.5)
        love.graphics.line(
            checkpoint.x + checkpoint.width / 2,
            checkpoint.y + 6,
            checkpoint.x + checkpoint.width / 2,
            checkpoint.y + checkpoint.height - 6
        )
        if checkpoint.active then
            love.graphics.setColor(0.92, 0.96, 1, 0.88)
            love.graphics.printf("SAVE", checkpoint.x - 10, checkpoint.y - 16, checkpoint.width + 20, "center")
        end
    end

    for _, anchor in ipairs(self.grapple_anchors) do
        love.graphics.setColor(
            self.accent_color[1],
            self.accent_color[2],
            self.accent_color[3],
            0.22
        )
        love.graphics.circle("fill", anchor.x, anchor.y, anchor.radius + 4)
        love.graphics.setColor(0.96, 0.94, 0.84, 1)
        love.graphics.circle("fill", anchor.x, anchor.y, anchor.radius)
        love.graphics.setColor(0.18, 0.16, 0.14, 1)
        love.graphics.circle("line", anchor.x, anchor.y, anchor.radius - 3)
        love.graphics.setColor(0.18, 0.16, 0.14, 0.7)
        love.graphics.line(anchor.x - 4, anchor.y, anchor.x + 4, anchor.y)
        love.graphics.line(anchor.x, anchor.y - 4, anchor.x, anchor.y + 4)
    end

    -- Reach zone beacon
    if self.reach_zone then
        local rz = self.reach_zone
        local pulse = 0.4 + 0.3 * math.sin(love.timer.getTime() * 4)
        -- Glow
        love.graphics.setColor(
            self.accent_color[1],
            self.accent_color[2],
            self.accent_color[3],
            0.12 * pulse
        )
        love.graphics.rectangle("fill", rz.x - 4, rz.y - 4, rz.width + 8, rz.height + 8, 4, 4)
        -- Border
        love.graphics.setColor(
            self.accent_color[1],
            self.accent_color[2],
            self.accent_color[3],
            0.6 * pulse
        )
        love.graphics.rectangle("line", rz.x, rz.y, rz.width, rz.height, 3, 3)
        -- Arrow indicator
        local cx = rz.x + rz.width / 2
        local cy = rz.y - 8
        love.graphics.setColor(
            self.accent_color[1],
            self.accent_color[2],
            self.accent_color[3],
            0.8 * pulse
        )
        love.graphics.polygon("fill", cx, cy, cx - 5, cy - 8, cx + 5, cy - 8)
    end

    for _, npc in ipairs(self.npcs) do
        local pulse = npc.active_hint and (0.18 + 0.06 * math.sin(love.timer.getTime() * 5)) or 0.1
        love.graphics.setColor(self.accent_color[1], self.accent_color[2], self.accent_color[3], pulse)
        love.graphics.circle("fill", npc.x + npc.width / 2, npc.y + npc.height / 2 + 1, 18)
        love.graphics.setColor(0.18, 0.18, 0.22, 0.85)
        love.graphics.rectangle("fill", npc.x + 8, npc.y + 10, npc.width - 16, npc.height - 6, 4, 4)
        love.graphics.setColor(0.8, 0.82, 0.95, 1)
        love.graphics.rectangle("fill", npc.x + 10, npc.y + 12, npc.width - 20, npc.height - 10, 4, 4)
        love.graphics.setColor(0.98, 0.88, 0.7, 1)
        love.graphics.circle("fill", npc.x + npc.width / 2, npc.y + 9, 5)
        if npc.active_hint then
            love.graphics.setColor(0.96, 0.96, 1, 0.85)
            love.graphics.printf(npc.speaker or "Guide", npc.x - 10, npc.y - 14, npc.width + 20, "center")
        end
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function Level:get_tile(px, py)
    if
        px < 0
        or py < 0
        or px >= self.map_width * TILE_SIZE
        or py >= self.map_height * TILE_SIZE
    then
        return 1
    end
    local tile_x = math.floor(px / TILE_SIZE) + 1
    local tile_y = math.floor(py / TILE_SIZE) + 1
    return self.map[tile_y] and self.map[tile_y][tile_x] or 0
end

function Level:is_solid_at_pixel(px, py)
    local tile_id = self:get_tile(px, py)
    return tile_id == 1 or tile_id == 2
end

function Level:check_gate_collision(player)
    if not self.show_gate or not self.gate then
        return false
    end

    return player.x < self.gate.x + self.gate.width
        and player.x + player.width > self.gate.x
        and player.y < self.gate.y + self.gate.height
        and player.y + player.height > self.gate.y
end

function Level:handle_hazards_collision(player)
    if self:get_tile(player.x + player.width / 2, player.y + player.height / 2) == 2 then
        return true
    end
    return false
end

function Level:update(dt, player)
    for _, trap in ipairs(self.timed_traps) do
        trap.timer = trap.timer - dt
        if trap.timer <= 0 then
            if trap.state == "off" then
                trap.state = "warning"
                trap.timer = trap.warning_duration
            elseif trap.state == "warning" then
                trap.state = "on"
                trap.timer = trap.on_duration
            else
                trap.state = "off"
                trap.timer = trap.off_duration
            end
        end
    end

    for _, platform in ipairs(self.moving_platforms) do
        local old_x = platform.x
        if platform.direction == 1 then
            platform.x = platform.x + platform.speed * dt
            if platform.x >= platform.end_x then
                platform.x = platform.end_x
                platform.direction = -1
            end
        else
            platform.x = platform.x - platform.speed * dt
            if platform.x <= platform.start_x then
                platform.x = platform.start_x
                platform.direction = 1
            end
        end
        platform.dx = platform.x - old_x
    end

    for key, block in pairs(self.crumbling_blocks) do
        if block.state == "crumbling" then
            block.timer = block.timer - dt
            if block.timer <= 0 then
                self.map[block.y][block.x] = 0
                self.crumbling_blocks[key] = nil
            end
        end
    end

    if player then
        self:update_npcs(player)
    end
end

function Level:update_npcs(player)
    local ui = self.services and self.services.ui or g_ui
    local player_center_x = player.x + player.width / 2
    local player_center_y = player.y + player.height / 2

    for _, npc in ipairs(self.npcs) do
        local npc_center_x = npc.x + npc.width / 2
        local npc_center_y = npc.y + npc.height / 2
        local radius = npc.trigger_radius or 60
        local in_range = distance_sq(player_center_x, player_center_y, npc_center_x, npc_center_y) <= (radius * radius)
        npc.active_hint = in_range

        if in_range and not npc.triggered and npc.line and ui and ui.show_story_callout then
            npc.triggered = true
            ui:show_story_callout(npc.speaker or self.scene_title, npc.line, self.accent_color, 3.2)
            local mark_meaningful_action = get_mark_meaningful_action(self)
            if mark_meaningful_action then
                mark_meaningful_action()
            end
        end
    end
end

function Level:handle_tile_effects(player, dt)
    player.is_slowed = self:get_tile(player.x + player.width / 2, player.y + player.height + 1) == 3
    player.environment_force_x = 0
    player.environment_force_y = 0
    player.environment_label = nil

    local center_x = player.x + player.width / 2
    local center_y = player.y + player.height / 2

    for _, wind in ipairs(self.wind_areas) do
        if
            center_x >= wind.x
            and center_x <= wind.x + wind.width
            and center_y >= wind.y
            and center_y <= wind.y + wind.height
        then
            local force_x = wind.force_x or 0
            local force_y = wind.force_y or 0
            if player.is_crouching and player.on_ground then
                force_x = force_x * 0.28
                force_y = force_y * 0.55
            end
            player.environment_force_x = player.environment_force_x + force_x
            player.environment_force_y = player.environment_force_y + force_y
            player.environment_label = wind.label or self.environment_hook
            if player.environment_label and player.environment_label ~= "" then
                player.seen_environment_labels = player.seen_environment_labels or {}
                if not player.seen_environment_labels[player.environment_label] then
                    player.seen_environment_labels[player.environment_label] = true
                    local ui = self.services and self.services.ui or g_ui
                    if ui then
                        local subtitle = self.environment_hook or player.environment_label
                        if self.removed_ability ~= "none" then
                            subtitle = "No " .. self.removed_ability .. ". " .. subtitle
                        end
                        ui:show_story_callout(
                            self.scene_title,
                            subtitle,
                            self.accent_color,
                            1.8
                        )
                    end
                    local record_environment_label = get_record_environment_label(self)
                    if record_environment_label then
                        record_environment_label(player.environment_label)
                    end
                end
            end
            if dt and math.random() < dt * 12 then
                local effects = self.services and self.services.effects or g_effects
                effects:run_dust(center_x, center_y + math.random(-8, 8))
            end
        end
    end
end

function Level:get_grapple_anchor_at(px, py, radius)
    local threshold = radius or 16
    local threshold_sq = threshold * threshold
    for _, anchor in ipairs(self.grapple_anchors) do
        local dx = anchor.x - px
        local dy = anchor.y - py
        if dx * dx + dy * dy <= threshold_sq then
            return anchor
        end
    end
    return nil
end

function Level:check_checkpoint_collision(player)
    for _, checkpoint in ipairs(self.checkpoints) do
        local hit = player.x < checkpoint.x + checkpoint.width
            and player.x + player.width > checkpoint.x
            and player.y < checkpoint.y + checkpoint.height
            and player.y + player.height > checkpoint.y
        if hit and self.active_checkpoint ~= checkpoint then
            if self.active_checkpoint then
                self.active_checkpoint.active = false
            end
            checkpoint.active = true
            self.active_checkpoint = checkpoint
            local record_checkpoint_activated = get_record_checkpoint_activated(self)
            if record_checkpoint_activated then
                record_checkpoint_activated(checkpoint)
            end
            return checkpoint
        end
    end
    return nil
end

function Level:get_respawn_point()
    if self.active_checkpoint then
        return {
            x = self.active_checkpoint.x,
            y = self.active_checkpoint.y,
        }
    end
    return {
        x = self.start_pos.x,
        y = self.start_pos.y,
    }
end

function Level.reset(_self) end

function Level:get_pullable_block_at(world_x, world_y)
    for _, block in ipairs(self.pullable_blocks) do
        if
            world_x >= block.x
            and world_x < block.x + block.width
            and world_y >= block.y
            and world_y < block.y + block.height
        then
            return block
        end
    end
    return nil
end

return Level
