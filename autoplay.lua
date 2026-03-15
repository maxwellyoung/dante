local Autoplay = {}
Autoplay.__index = Autoplay

local function shell_quote(value)
    return "'" .. tostring(value):gsub("'", "'\\''") .. "'"
end

local function ensure_directory(path)
    if not path or path == "" then
        return
    end
    os.execute("mkdir -p " .. shell_quote(path))
end

local function join_path(base, leaf)
    if base:sub(-1) == "/" then
        return base .. leaf
    end
    return base .. "/" .. leaf
end

local function absolute_path(path)
    if not path or path == "" then
        return path
    end
    if path:sub(1, 1) == "/" then
        return path
    end
    local cwd = love.filesystem.getWorkingDirectory()
    return join_path(cwd, path)
end

local function parent_directory(path)
    return path:match("^(.*)/[^/]+$") or "."
end

local function round(value)
    return math.floor(value * 100 + 0.5) / 100
end

local function get_player(self)
    return self.services and self.services.player or g_player
end

local function get_level(self)
    return self.services and self.services.level or g_level
end

local function get_run_stats(self)
    return self.services and self.services.run_stats or g_run_stats
end

local function get_input_override(self)
    return self.services and self.services.input_override or g_input_override
end

local function get_select_flow_room(self)
    return self.services and self.services.select_flow_room or g_select_flow_room
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

function Autoplay:new(services, report_path, variant)
    local class = self or Autoplay
    local instance = setmetatable({}, class)
    instance.services = services
    instance.report_path = report_path or "tmp/autoplay-report.json"
    instance.variant = variant or "a"
    instance.profile = "proving-ground"
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
    instance.capture_in_flight = false
    instance.pending_transition = nil
    instance.pending_quit = false
    instance.room_started = false
    return instance
end

function Autoplay:set_screenshot_dir(path)
    self.screenshot_dir = absolute_path(path)
    ensure_directory(self.screenshot_dir)
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
    local target = self.screenshot_dir .. "/" .. filename
    self.screenshot_requests[#self.screenshot_requests + 1] = target
    local room = self.room_metrics[#self.room_metrics]
    if room then
        room.screenshots = room.screenshots or {}
        room.screenshots[label] = target
    end
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
    local player = get_player(self)
    local input_override = get_input_override(self)
    for key, value in pairs(self.keys) do
        local previous = self.previous_keys[key] == true
        if value and not previous then
            if player and player.keypressed then
                player:keypressed(key)
            end
        elseif not value and previous then
            if player and player.keyreleased then
                player:keyreleased(key)
            end
        end
        self.previous_keys[key] = value
    end

    for key, previous in pairs(self.previous_keys) do
        if self.keys[key] == nil and previous then
            if player and player.keyreleased then
                player:keyreleased(key)
            end
            self.previous_keys[key] = false
        end
    end

    for key in pairs(input_override) do
        input_override[key] = nil
    end
    for key, value in pairs(self.keys) do
        if value then
            input_override[key] = true
        end
    end
end

function Autoplay:flush_actions()
    local player = get_player(self)
    for _, action in ipairs(self.pending_actions) do
        if action.type == "shoot" then
            player:fire_weapon(action.x, action.y)
        elseif action.type == "grapple" then
            player:fire_grapple(action.x, action.y)
        elseif action.type == "grapple_auto" then
            player:fire_grapple_auto()
        elseif action.type == "dash" then
            player:dash()
        elseif action.type == "roll" then
            player:roll()
        end
    end
    self.pending_actions = {}
end

function Autoplay:on_scene_loaded(scene, room_index)
    local previous = self.room_metrics[#self.room_metrics]
    if previous and previous.result == nil then
        table.remove(self.room_metrics, #self.room_metrics)
    end

    self.current_room = scene
    self.current_room_index = room_index or 1
    self.timer = 0
    self.room_started = true
    self.keys = {}
    self.previous_keys = {}
    self.pending_actions = {}
    self.pending_transition = nil
    self.jump_one = false
    self.jump_two = false
    self.jump_one_started_at = nil
    self.jump_two_started_at = nil
    self.grapple_one = false
    self.grapple_two = false
    self.grapple_three = false
    self.grapple_retry = false
    self.last_traversal_grapple_attempt_at = nil
    self.last_traversal_jump_started_at = nil
    self.pressure_jump_one = false
    self.pressure_jump_two = false
    self.pressure_jump_one_started_at = nil
    self.pressure_jump_two_started_at = nil
    self.dash_lane_dash_one = false
    self.dash_lane_jump_one = false
    self.dash_lane_jump_one_started_at = nil
    self.ricochet_shot_fired = false
    self.last_shot_time = -1
    self.room_metrics[#self.room_metrics + 1] = {
        room = scene.id or tostring(room_index),
        title = scene.title,
        variant = self.variant,
        qa_expectations = scene.qa_expectations or {},
        objective_text = scene.objective_text or "",
        started_at = round(get_run_stats(self) and get_run_stats(self).total_time or 0),
        started_resets = get_run_stats(self) and get_run_stats(self).resets or 0,
        screenshots = {},
    }
    self:queue_screenshot("start")
end

local function evaluate_room(room)
    local expectations = room.qa_expectations or {}
    local reasons = {}
    local accuracy = 0
    if (room.shots_fired or 0) > 0 then
        accuracy = (room.shots_hit / room.shots_fired) * 100
    end

    if expectations.required_result and room.result ~= expectations.required_result then
        reasons[#reasons + 1] = "expected_result=" .. expectations.required_result
    end
    if expectations.max_deaths ~= nil and (room.deaths or 0) > expectations.max_deaths then
        reasons[#reasons + 1] = "max_deaths=" .. tostring(expectations.max_deaths)
    end
    if expectations.min_grapple_latches ~= nil and (room.grapple_latches or 0) < expectations.min_grapple_latches then
        reasons[#reasons + 1] = "min_grapple_latches=" .. tostring(expectations.min_grapple_latches)
    end
    if expectations.min_shots_hit ~= nil and (room.shots_hit or 0) < expectations.min_shots_hit then
        reasons[#reasons + 1] = "min_shots_hit=" .. tostring(expectations.min_shots_hit)
    end
    if expectations.min_shot_ricochets ~= nil and (room.shot_ricochets or 0) < expectations.min_shot_ricochets then
        reasons[#reasons + 1] = "min_shot_ricochets=" .. tostring(expectations.min_shot_ricochets)
    end
    if expectations.min_bank_kills ~= nil and (room.shot_bank_kills or 0) < expectations.min_bank_kills then
        reasons[#reasons + 1] = "min_bank_kills=" .. tostring(expectations.min_bank_kills)
    end
    if expectations.min_accuracy ~= nil and accuracy < expectations.min_accuracy then
        reasons[#reasons + 1] = "min_accuracy=" .. tostring(expectations.min_accuracy)
    end
    if expectations.min_distance ~= nil and (room.distance or 0) < expectations.min_distance then
        reasons[#reasons + 1] = "min_distance=" .. tostring(expectations.min_distance)
    end
    if expectations.min_dashes ~= nil and (room.dashes or 0) < expectations.min_dashes then
        reasons[#reasons + 1] = "min_dashes=" .. tostring(expectations.min_dashes)
    end
    if expectations.min_rolls ~= nil and (room.rolls or 0) < expectations.min_rolls then
        reasons[#reasons + 1] = "min_rolls=" .. tostring(expectations.min_rolls)
    end
    if expectations.min_crouch_time ~= nil and (room.crouch_time or 0) < expectations.min_crouch_time then
        reasons[#reasons + 1] = "min_crouch_time=" .. tostring(expectations.min_crouch_time)
    end
    if expectations.requires_encounter_clear and not room.encounter_complete then
        reasons[#reasons + 1] = "encounter_not_complete"
    end
    if expectations.requires_removed_ability_notice and not room.removed_ability_notice_shown then
        reasons[#reasons + 1] = "removed_ability_notice_missing"
    end
    if expectations.requires_environment_label and not room.environment_label_seen then
        reasons[#reasons + 1] = "environment_label_missing"
    end

    room.accuracy = round(accuracy)
    room.pass = #reasons == 0
    room.fail_reasons = reasons
end

function Autoplay:record_room_result(result)
    local room = self.room_metrics[#self.room_metrics]
    local player = get_player(self)
    local run_stats = get_run_stats(self)
    if not room then
        return
    end

    room.result = result
    room.duration = round(self.timer)
    room.player_x = round(player.x)
    room.player_y = round(player.y)
    room.deaths = run_stats.room_deaths
    room.shots_fired = run_stats.room_shots_fired
    room.shots_hit = run_stats.room_shots_hit
    room.shot_ricochets = run_stats.room_shot_ricochets or 0
    room.shot_bank_kills = run_stats.room_shot_bank_kills or 0
    room.jumps = run_stats.room_jumps or 0
    room.grapples_fired = run_stats.room_grapples_fired or 0
    room.grapple_latches = run_stats.room_grapple_latches or 0
    room.dashes = run_stats.room_dashes or 0
    room.rolls = run_stats.room_rolls or 0
    room.crouch_time = round(run_stats.room_crouch_time or 0)
    room.distance = round(run_stats.room_distance or 0)
    room.resets = (run_stats.resets or 0) - (room.started_resets or 0)
    room.first_meaningful_action_at = round(run_stats.room_first_meaningful_action_at or -1)
    room.environment_label_seen = run_stats.room_environment_label_seen or false
    room.environment_label = run_stats.room_environment_label
    room.removed_ability_notice_shown = run_stats.room_removed_ability_notice_shown or false
    room.encounter_complete = run_stats.room_encounter_complete or false
    self:queue_screenshot(result)
    evaluate_room(room)
end

function Autoplay:complete_room(result, transition)
    if self.pending_transition then
        return
    end
    self:record_room_result(result)
    self.pending_transition = transition
end

function Autoplay:advance_after_capture()
    local transition = self.pending_transition
    local select_flow_room = get_select_flow_room(self)
    self.pending_transition = nil
    if not transition then
        return
    end

    if transition.type == "room" then
        select_flow_room(transition.index)
    elseif transition.type == "finish" then
        self:finish()
    end
end

function Autoplay:finish()
    if self.pending_quit then
        return
    end
    local run_stats = get_run_stats(self)
    local overall_pass = true
    for _, room in ipairs(self.room_metrics) do
        if room.pass == false then
            overall_pass = false
            break
        end
    end

    local report = {
        profile = self.profile,
        total_time = round(run_stats.total_time),
        variant = self.variant,
        overall_pass = overall_pass,
        rooms = self.room_metrics,
        totals = {
            deaths = run_stats.deaths,
            shots_fired = run_stats.shots_fired,
            shots_hit = run_stats.shots_hit,
            shot_ricochets = run_stats.shot_ricochets or 0,
            shot_bank_kills = run_stats.shot_bank_kills or 0,
            jumps = run_stats.jumps or 0,
            grapples_fired = run_stats.grapples_fired or 0,
            grapple_latches = run_stats.grapple_latches or 0,
            dashes = run_stats.dashes or 0,
            rolls = run_stats.rolls or 0,
            crouch_time = round(run_stats.crouch_time or 0),
            distance = round(run_stats.distance or 0),
            first_meaningful_action_at = round(run_stats.first_meaningful_action_at or -1),
            resets = run_stats.resets or 0,
        },
    }

    local encoded = encode_table(report)
    local report_path = absolute_path(self.report_path)
    ensure_directory(parent_directory(report_path))
    local handle = io.open(report_path, "w")
    if handle then
        handle:write(encoded)
        handle:close()
    end
    print(encoded)
    self:queue_screenshot("summary")
    self.pending_quit = true
end

function Autoplay:process_screenshot()
    if self.capture_in_flight then
        return true
    end

    if #self.screenshot_requests > 0 then
        local target = table.remove(self.screenshot_requests, 1)
        self.capture_in_flight = true
        love.graphics.captureScreenshot(function(image_data)
            local ok, file_data = pcall(function()
                return image_data:encode("png")
            end)

            if ok and file_data then
                local handle = io.open(target, "wb")
                if handle then
                    handle:write(file_data:getString())
                    handle:close()
                else
                    print("Autoplay screenshot write failed: " .. target)
                end
            else
                print("Autoplay screenshot encode failed: " .. tostring(file_data))
            end

            self.capture_in_flight = false
        end)
        return true
    end

    if self.pending_transition and not self.capture_in_flight then
        self:advance_after_capture()
        return true
    end

    if self.pending_quit and not self.capture_in_flight then
        self.finished = true
        os.exit(0)
        return true
    end

    return false
end

function Autoplay:queue_grapple_to_anchor(index)
    local level = get_level(self)
    local anchors = level and level.grapple_anchors or nil
    if not anchors or not anchors[index] then
        return false
    end
    local anchor = anchors[index]
    self:queue_action({ type = "grapple", x = anchor.x, y = anchor.y })
    return true
end

function Autoplay:try_traversal_grapple()
    local player = get_player(self)
    local run_stats = get_run_stats(self)
    if (run_stats.room_grapple_latches or 0) > 0 then
        return false
    end
    if not player or player.grapple_state ~= "ready" then
        return false
    end
    if self.last_traversal_grapple_attempt_at and (self.timer - self.last_traversal_grapple_attempt_at) < 0.35 then
        return false
    end
    if player.x < 180 then
        return false
    end

    self.last_traversal_grapple_attempt_at = self.timer
    if not self:queue_grapple_to_anchor(1) then
        self:queue_action({ type = "grapple_auto" })
    end
    return true
end

function Autoplay:run_traversal_room()
    self:set_key("d", true)
    g_player.facing = 1
    g_player.vx = math.max(g_player.vx or 0, 220)

    if self.variant == "c" and self.timer > 0.16 and not self.jump_one and g_player.on_ground then
        self.jump_one = true
        self.jump_one_started_at = self.timer
        self:press_jump()
    elseif self.jump_one and self.jump_one_started_at and (self.timer - self.jump_one_started_at) > 0.12 then
        self:release_jump()
    end

    if (g_run_stats.room_grapple_latches or 0) == 0 and self.timer > 0.35 then
        if self.variant ~= "c" or self.timer > 0.7 then
            self:try_traversal_grapple()
        end
    end

    if (g_run_stats.room_grapple_latches or 0) == 0 and self.timer > 1.35 then
        self:queue_grapple_to_anchor(1)
    end

    local gate_x = (g_level and g_level.gate and g_level.gate.x) or 1240
    local reached_exit = (g_run_stats.room_grapple_latches or 0) > 0
        and (g_player.x + g_player.width) > (gate_x - 28)

    if reached_exit then
        self:complete_room("movement_complete", { type = "room", index = 2 })
    elseif self.timer > 7.2 then
        self:complete_room("movement_failed", {
            type = "room",
            index = 2,
        })
    end
end

function Autoplay:run_grapple_room()
    self:set_key("d", true)

    if g_player.x > 120 and not self.grapple_one then
        self.grapple_one = true
        self:queue_grapple_to_anchor(1)
    end

    if (g_run_stats.room_grapple_latches or 0) == 0 and self.timer > 1.3 and not self.grapple_retry then
        self.grapple_retry = true
        if self.variant == "c" then
            self:queue_grapple_to_anchor(2)
        else
            self:queue_grapple_to_anchor(self.variant == "b" and 2 or 1)
        end
    end

    if (g_run_stats.room_grapple_latches or 0) >= 1 and g_player.x > 620 and not self.grapple_two then
        self.grapple_two = true
        if self.variant == "c" then
            self:queue_grapple_to_anchor(3)
        else
            self:queue_grapple_to_anchor(self.variant == "b" and 3 or 2)
        end
    end

    local third_grapple_threshold = self.variant == "c" and 860 or 1030
    if (g_run_stats.room_grapple_latches or 0) >= 2 and g_player.x > third_grapple_threshold and not self.grapple_three then
        self.grapple_three = true
        self:queue_grapple_to_anchor(3)
    end

    local success = (g_run_stats.room_grapple_latches or 0) >= 2 and g_player.x > 1440
    if success then
        self:complete_room("grapple_complete", { type = "room", index = 5 })
    elseif self.timer > (self.variant == "c" and 9.0 or 8.4) then
        self:complete_room("grapple_failed", { type = "room", index = 5 })
    end
end

function Autoplay:run_dash_room()
    self:set_key("d", true)
    g_player.facing = 1
    g_player.vx = math.max(g_player.vx or 0, 220)

    if g_player.x > 430 and not self.dash_lane_dash_one and g_player.on_ground then
        self.dash_lane_dash_one = true
        self:queue_action({ type = "dash" })
    end

    if g_player.x > 1040 and not self.dash_lane_jump_one and g_player.on_ground then
        self.dash_lane_jump_one = true
        self.dash_lane_jump_one_started_at = self.timer
        self:press_jump()
    elseif self.dash_lane_jump_one
        and self.dash_lane_jump_one_started_at
        and (self.timer - self.dash_lane_jump_one_started_at) > 0.14 then
        self:release_jump()
    end

    local gate_x = (g_level and g_level.gate and g_level.gate.x) or 1312
    local reached_exit = (g_run_stats.room_dashes or 0) >= 1
        and (g_player.x + g_player.width) > (gate_x - 28)

    if reached_exit then
        self:complete_room("dash_complete", { type = "room", index = 6 })
    elseif self.timer > 6.5 then
        self:complete_room("dash_failed", { type = "room", index = 6 })
    end
end

function Autoplay:run_ricochet_room()
    local firing_x = self.variant == "c" and 162 or 158
    local first_bank_target_x = self.variant == "c" and 208 or 204
    local first_bank_target_y = self.variant == "c" and 288 or 288
    local retry_bank_target_x = self.variant == "c" and 212 or 216
    local retry_bank_target_y = self.variant == "c" and 284 or 280

    if (g_run_stats.room_shot_bank_kills or 0) >= 1 then
        self:set_key("d", true)
        g_player.vx = math.max(g_player.vx or 0, 180)
    elseif g_player.x < firing_x then
        self:set_key("d", true)
    else
        self:set_key("d", false)
        g_player.x = firing_x
        g_player.vx = 0
    end

    g_player.facing = 1

    if g_player.x >= firing_x and not self.ricochet_shot_fired and g_player.on_ground then
        self.ricochet_shot_fired = true
        self.last_shot_time = self.timer
        self:queue_action({
            type = "shoot",
            x = first_bank_target_x,
            y = first_bank_target_y,
        })
    end

    if (g_run_stats.room_shot_bank_kills or 0) < 1
        and self.ricochet_shot_fired
        and self.timer - self.last_shot_time > (self.variant == "c" and 0.95 or 0.72) then
        self.last_shot_time = self.timer
        self:queue_action({
            type = "shoot",
            x = retry_bank_target_x,
            y = retry_bank_target_y,
        })
    end

    local gate_x = (g_level and g_level.gate and g_level.gate.x) or 1312
    local reached_exit = (g_run_stats.room_shot_bank_kills or 0) >= 1
        and (g_run_stats.room_shot_ricochets or 0) >= 1
        and (g_player.x + g_player.width) > (gate_x - 28)

    if reached_exit then
        self:complete_room("ricochet_complete", { type = "room", index = 4 })
    elseif self.timer > 7.8 then
        self:complete_room("ricochet_failed", { type = "room", index = 4 })
    end
end

function Autoplay:run_posture_room()
    self:set_key("d", true)
    if self.timer < (self.variant == "c" and 0.95 or 0.62) then
        self:set_key("s", true)
    else
        self:set_key("s", false)
    end

    if g_player.x > 420 and not self.dash_lane_dash_one and g_player.on_ground then
        self.dash_lane_dash_one = true
        self:queue_action({ type = "roll" })
    end

    if self.variant == "c" and g_player.x > 780 and not self.dash_lane_jump_one and g_player.on_ground then
        self.dash_lane_jump_one = true
        self.dash_lane_jump_one_started_at = self.timer
        self:press_jump()
    elseif self.dash_lane_jump_one
        and self.dash_lane_jump_one_started_at
        and (self.timer - self.dash_lane_jump_one_started_at) > 0.14 then
        self:release_jump()
    end

    local gate_x = (g_level and g_level.gate and g_level.gate.x) or 1248
    local reached_exit = (g_run_stats.room_rolls or 0) >= 1
        and (g_run_stats.room_crouch_time or 0) >= 0.3
        and (g_player.x + g_player.width) > (gate_x - 28)

    if reached_exit then
        self:complete_room("posture_complete", { type = "finish" })
    elseif self.timer > 7.5 then
        self:complete_room("posture_failed", { type = "finish" })
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
        local shot_delay = self.variant == "c" and 0.24 or 0.16
        local fire_window = self.variant ~= "c" or self.timer > 1.1
        if fire_window and nearest_distance < 250 and self.timer - self.last_shot_time > shot_delay then
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

    local encounter_started = g_encounters and g_encounters.state ~= "idle" and g_encounters.state ~= "disabled"
    local encounter_complete = g_run_stats.room_encounter_complete or (g_encounters and g_encounters.state == "complete")
    if encounter_complete and #g_enemies.items == 0 and g_player.x > 1300 then
        self:complete_room("combat_complete", { type = "room", index = 3 })
    elseif encounter_started and self.timer > 9.0 then
        self:complete_room("combat_failed", { type = "room", index = 3 })
    end
end

function Autoplay:run_pressure_room()
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

    self:set_key("d", true)

    if g_player.x > 215 and not self.pressure_jump_one and g_player.on_ground then
        self.pressure_jump_one = true
        self.pressure_jump_one_started_at = self.timer
        self:press_jump()
    elseif self.pressure_jump_one and self.pressure_jump_one_started_at and (self.timer - self.pressure_jump_one_started_at) > 0.16 then
        self:release_jump()
    end

    if g_player.x > 560 and not self.pressure_jump_two and g_player.on_ground then
        self.pressure_jump_two = true
        self.pressure_jump_two_started_at = self.timer
        self:press_jump()
    elseif self.pressure_jump_two and self.pressure_jump_two_started_at and (self.timer - self.pressure_jump_two_started_at) > 0.18 then
        self:release_jump()
    end

    if nearest_enemy then
        if nearest_enemy.x > g_player.x + 170 then
            self:set_key("d", true)
        elseif nearest_distance < 150 and g_player.on_ground then
            self:set_key("d", false)
        end

        if self.variant == "c" and nearest_distance < 165 and g_player.on_ground and not self.dash_lane_dash_one then
            self.dash_lane_dash_one = true
            self:queue_action({ type = "dash" })
        end

        if nearest_distance < 240 and self.timer - self.last_shot_time > (self.variant == "c" and 0.2 or 0.16) then
            self.last_shot_time = self.timer
            self:queue_action({
                type = "shoot",
                x = nearest_enemy.x + nearest_enemy.width / 2,
                y = nearest_enemy.y + nearest_enemy.height / 2,
            })
        end
    end

    local encounter_started = g_encounters and g_encounters.state ~= "idle" and g_encounters.state ~= "disabled"
    local encounter_complete = g_run_stats.room_encounter_complete or (g_encounters and g_encounters.state == "complete")
    if encounter_complete and #g_enemies.items == 0 and g_player.x > 900 then
        self:complete_room("pressure_complete", { type = "room", index = 7 })
    elseif encounter_started and self.timer > 9.2 then
        self:complete_room("pressure_failed", { type = "room", index = 7 })
    end
end

function Autoplay:update(dt)
    if self.finished or not self.current_room then
        return
    end

    self.timer = self.timer + dt
    self:apply_keys()

    if self.pending_transition then
        self:flush_actions()
        return
    end

    if self.current_room.id == "prove_traversal" then
        self:run_traversal_room()
    elseif self.current_room.id == "prove_ricochet" then
        self:run_ricochet_room()
    elseif self.current_room.id == "prove_grapple" then
        self:run_grapple_room()
    elseif self.current_room.id == "prove_dash" then
        self:run_dash_room()
    elseif self.current_room.id == "prove_combat" then
        self:run_combat_room()
    elseif self.current_room.id == "prove_pressure" then
        self:run_pressure_room()
    elseif self.current_room.id == "prove_posture" then
        self:run_posture_room()
    end

    self:flush_actions()
end

return Autoplay
