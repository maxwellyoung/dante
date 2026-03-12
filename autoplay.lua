local Autoplay = {}
Autoplay.__index = Autoplay

local function round(value)
    return math.floor(value * 100 + 0.5) / 100
end

local function encode_table(value)
    local kind = type(value)
    if kind == "table" then
        local is_array = true
        local count = 0
        for key in pairs(value) do
            count = count + 1
            if type(key) ~= "number" then
                is_array = false
                break
            end
        end

        local parts = {}
        if is_array then
            for index = 1, count do
                parts[#parts + 1] = encode_table(value[index])
            end
            return "[" .. table.concat(parts, ",") .. "]"
        end

        for key, inner in pairs(value) do
            parts[#parts + 1] = string.format("%q:%s", tostring(key), encode_table(inner))
        end
        return "{" .. table.concat(parts, ",") .. "}"
    end

    if kind == "string" then
        return string.format("%q", value)
    end
    if kind == "number" or kind == "boolean" then
        return tostring(value)
    end
    return "null"
end

function Autoplay:new(report_path)
    local class = self or Autoplay
    local instance = setmetatable({}, class)
    instance.report_path = report_path or "tmp/autoplay-report.json"
    instance.screenshot_dir = nil
    instance.room_metrics = {}
    instance.current_room = nil
    instance.current_room_index = 1
    instance.timer = 0
    instance.completed = false
    instance.finished = false
    instance.keys = {}
    instance.previous_keys = {}
    instance.pending_actions = {}
    instance.screenshot_requests = {}
    instance.pending_quit = false
    instance.room_started = false
    return instance
end

function Autoplay:set_screenshot_dir(path)
    self.screenshot_dir = path
end

local function sanitize_filename(value)
    return (value:gsub("[^%w%-_]+", "-"):gsub("%-+", "-"))
end

function Autoplay:queue_screenshot(label)
    if not self.screenshot_dir or self.screenshot_dir == "" then
        return
    end
    local prefix = string.format("%02d-%s", self.current_room_index or 0, sanitize_filename(self.current_room and self.current_room.id or "room"))
    local filename = prefix .. "-" .. sanitize_filename(label) .. ".png"
    self.screenshot_requests[#self.screenshot_requests + 1] = self.screenshot_dir .. "/" .. filename
end

function Autoplay:set_key(key, value)
    self.keys[key] = value == true
end

function Autoplay:press_jump()
    self.keys.space = true
end

function Autoplay:release_jump()
    self.keys.space = false
end

function Autoplay:queue_action(action)
    self.pending_actions[#self.pending_actions + 1] = action
end

function Autoplay:apply_keys()
    for key, value in pairs(self.keys) do
        local previous = self.previous_keys[key] == true
        if value and not previous then
            if g_player and g_player.keypressed then
                g_player:keypressed(key)
            end
        elseif not value and previous then
            if g_player and g_player.keyreleased then
                g_player:keyreleased(key)
            end
        end
        self.previous_keys[key] = value
    end

    for key, previous in pairs(self.previous_keys) do
        if self.keys[key] == nil and previous then
            if g_player and g_player.keyreleased then
                g_player:keyreleased(key)
            end
            self.previous_keys[key] = false
        end
    end

    for key in pairs(g_input_override) do
        g_input_override[key] = nil
    end
    for key, value in pairs(self.keys) do
        if value then
            g_input_override[key] = true
        end
    end
end

function Autoplay:flush_actions()
    for _, action in ipairs(self.pending_actions) do
        if action.type == "shoot" then
            g_player:fire_weapon(action.x, action.y)
        elseif action.type == "grapple" then
            g_player:fire_grapple(action.x, action.y)
        end
    end
    self.pending_actions = {}
end

function Autoplay:on_scene_loaded(scene, room_index)
    self.current_room = scene
    self.current_room_index = room_index or 1
    self.timer = 0
    self.room_started = true
    self.keys = {}
    self.previous_keys = {}
    self.pending_actions = {}
    self.jump_one = false
    self.jump_two = false
    self.grapple_one = false
    self.pressure_jump_one = false
    self.pressure_jump_two = false
    self.last_shot_time = -1
    self:queue_screenshot("start")
    self.room_metrics[#self.room_metrics + 1] = {
        room = scene.id or tostring(room_index),
        title = scene.title,
        started_at = round(g_run_stats and g_run_stats.total_time or 0),
    }
end

function Autoplay:record_room_result(result)
    local room = self.room_metrics[#self.room_metrics]
    if not room then
        return
    end

    room.result = result
    room.duration = round(self.timer)
    room.player_x = round(g_player.x)
    room.player_y = round(g_player.y)
    room.deaths = g_run_stats.room_deaths
    room.shots_fired = g_run_stats.room_shots_fired
    room.shots_hit = g_run_stats.room_shots_hit
    room.jumps = g_run_stats.room_jumps or 0
    room.grapples_fired = g_run_stats.room_grapples_fired or 0
    room.grapple_latches = g_run_stats.room_grapple_latches or 0
    room.distance = round(g_run_stats.room_distance or 0)
    self:queue_screenshot(result)
end

function Autoplay:finish()
    if self.pending_quit then
        return
    end
    local report = {
        total_time = round(g_run_stats.total_time),
        rooms = self.room_metrics,
        totals = {
            deaths = g_run_stats.deaths,
            shots_fired = g_run_stats.shots_fired,
            shots_hit = g_run_stats.shots_hit,
            jumps = g_run_stats.jumps or 0,
            grapples_fired = g_run_stats.grapples_fired or 0,
            grapple_latches = g_run_stats.grapple_latches or 0,
            distance = round(g_run_stats.distance or 0),
        },
    }

    local encoded = encode_table(report)
    os.execute("mkdir -p tmp")
    local handle = io.open(self.report_path, "w")
    if handle then
        handle:write(encoded)
        handle:close()
    end
    print(encoded)
    self:queue_screenshot("summary")
    self.pending_quit = true
end

function Autoplay:process_screenshot()
    if #self.screenshot_requests > 0 then
        local target = table.remove(self.screenshot_requests, 1)
        love.graphics.captureScreenshot(target)
        return true
    end

    if self.pending_quit then
        self.finished = true
        love.event.quit()
        return true
    end

    return false
end

function Autoplay:run_traversal_room()
    self:set_key("d", true)

    if g_player.x > 190 and not self.jump_one then
        self.jump_one = true
        self:press_jump()
    elseif self.jump_one and self.timer > 0.55 then
        self:release_jump()
    end

    if g_player.x > 450 and not self.jump_two then
        self.jump_two = true
        self:press_jump()
    elseif self.jump_two and self.timer > 1.35 then
        self:release_jump()
    end

    if g_player.x > 900 and not self.grapple_one then
        self.grapple_one = true
        self:queue_action({ type = "grapple", x = 1200, y = 240 })
    end

    if self.timer > 5.2 then
        self:record_room_result("movement_failed")
        g_select_flow_room(2)
    elseif g_player.x > 1260 and (g_run_stats.room_grapple_latches or 0) > 0 then
        self:record_room_result("movement_complete")
        g_select_flow_room(2)
    end
end

function Autoplay:run_combat_room()
    local nearest_enemy = nil
    local nearest_distance = math.huge
    local player_center_x = g_player.x + g_player.width / 2

    for _, enemy in ipairs(g_enemies.items) do
        local dx = enemy.x - player_center_x
        local abs_dx = math.abs(dx)
        if abs_dx < nearest_distance then
            nearest_distance = abs_dx
            nearest_enemy = enemy
        end
    end

    if nearest_enemy then
        if nearest_enemy.x > g_player.x + 240 then
            self:set_key("d", true)
        else
            self:set_key("d", false)
        end
        if nearest_distance < 250 and self.timer - self.last_shot_time > 0.16 then
            self.last_shot_time = self.timer
            self:queue_action({
                type = "shoot",
                x = nearest_enemy.x + nearest_enemy.width / 2,
                y = nearest_enemy.y + nearest_enemy.height / 2,
            })
        end
    else
        self:set_key("d", true)
    end

    if #g_enemies.items == 0 then
        self:record_room_result("combat_complete")
        g_select_flow_room(3)
    elseif self.timer > 8.0 then
        self:record_room_result("combat_failed")
        g_select_flow_room(3)
    end
end

function Autoplay:run_pressure_room()
    self:set_key("d", true)

    if g_player.x > 215 and not self.pressure_jump_one then
        self.pressure_jump_one = true
        self:press_jump()
    elseif self.pressure_jump_one and self.timer > 0.9 then
        self:release_jump()
    end

    if g_player.x > 560 and not self.pressure_jump_two then
        self.pressure_jump_two = true
        self:press_jump()
    elseif self.pressure_jump_two and self.timer > 2.2 then
        self:release_jump()
    end

    local nearest_enemy = g_enemies.items[1]
    if
        nearest_enemy
        and math.abs(nearest_enemy.x - g_player.x) < 220
        and self.timer - self.last_shot_time > 0.18
    then
        self.last_shot_time = self.timer
        self:queue_action({
            type = "shoot",
            x = nearest_enemy.x + nearest_enemy.width / 2,
            y = nearest_enemy.y + nearest_enemy.height / 2,
        })
    end

    if self.timer > 5.2 or g_player.x > 900 then
        self:record_room_result("pressure_complete")
        self:finish()
    end
end

function Autoplay:update(dt)
    if self.finished or not self.current_room then
        return
    end

    self.timer = self.timer + dt
    self:apply_keys()

    if self.current_room.id == "prove_traversal" then
        self:run_traversal_room()
    elseif self.current_room.id == "prove_combat" then
        self:run_combat_room()
    elseif self.current_room.id == "prove_pressure" then
        self:run_pressure_room()
    end

    self:flush_actions()
end

return Autoplay
