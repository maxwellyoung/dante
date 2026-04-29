local EncounterController = {}
EncounterController.__index = EncounterController

local function get_record_encounter_complete(self)
    return self.services and self.services.record_encounter_complete or g_record_encounter_complete
end

local function get_ui(self)
    return self.services and self.services.ui or g_ui
end

local function get_enemies(self)
    return self.services and self.services.enemies or g_enemies
end

local function get_camera(self)
    return self.services and self.services.camera or g_camera
end

local function get_effects(self)
    return self.services and self.services.effects or g_effects
end

local function get_player(self)
    return self.services and self.services.player or g_player
end

local function get_level(self)
    return self.services and self.services.level or g_level
end

local function copy_spawn(spawn)
    return {
        x = spawn.x,
        y = spawn.y,
        type = spawn.type,
    }
end

function EncounterController:new(services)
    local class = self or EncounterController
    local instance = setmetatable({}, class)
    instance.services = services
    instance.scene = nil
    instance.config = nil
    instance.state = "idle"
    instance.wave_index = 0
    instance.wave_delay = 0
    instance.beat = nil
    instance.beat_shown = false
    return instance
end

function EncounterController:load_scene(scene)
    self.scene = scene
    self.config = scene and scene.encounter_config or nil
    self.beat = scene and scene.beat_config or nil
    self.state = self.config and "idle" or "disabled"
    self.wave_index = 0
    self.wave_delay = 0
    self.beat_shown = false
end

function EncounterController:is_active()
    return self.state == "active" or self.state == "arming"
end

function EncounterController:is_complete()
    return self.state == "complete" or self.state == "disabled"
end

function EncounterController:get_progress()
    if not self.config or self.state == "disabled" then
        return nil
    end

    return {
        active = self:is_active(),
        wave = math.max(self.wave_index, self.state == "idle" and 0 or 1),
        total_waves = #(self.config.waves or {}),
        enemies_remaining = #(get_enemies(self).items),
    }
end

function EncounterController:has_blockers()
    return self:is_active() and self.config and self.config.bounds ~= nil
end

function EncounterController:get_bounds()
    return self.config and self.config.bounds or nil
end

function EncounterController:start()
    if not self.config or self.state ~= "idle" then
        return
    end
    self.state = "arming"
    self.wave_index = 0
    -- Longer arming delay for instant-trigger encounters so player reads the room
    local instant_trigger = (self.config.trigger_x or math.huge) <= 32
    self.wave_delay = instant_trigger and 1.0 or 0.18
    local scene_has_trial_rule = self.scene
        and self.scene.trial_rule
        and self.scene.trial_rule ~= ""
    if self.config.intro_title and not scene_has_trial_rule then
        local ui = get_ui(self)
        ui:show_banner(
            self.config.intro_title,
            self.config.intro_subtitle or "",
            self.scene.accent_color,
            1.7
        )
    end
end

function EncounterController:spawn_wave(wave)
    local ui = get_ui(self)
    local enemies = get_enemies(self)
    local camera = get_camera(self)
    local effects = get_effects(self)
    local player = get_player(self)
    if wave.banner then
        ui:show_banner(
            wave.banner,
            wave.subtitle or "",
            self.scene.accent_color,
            1.25
        )
    end
    for _, spawn in ipairs(wave.spawns or {}) do
        local entry = copy_spawn(spawn)
        enemies:spawn(entry.x, entry.y, entry.type)
    end
    camera:shake(4, 0.12)
    local bounds = self.config and self.config.bounds or {}
    local rumble_x = bounds.left and bounds.right and ((bounds.left + bounds.right) * 0.5)
        or (player.x + player.width / 2)
    effects:rumble(rumble_x, player.y + player.height)
end

function EncounterController:complete()
    if self.state == "complete" or self.state == "disabled" then
        return
    end
    self.state = "complete"
    local record_encounter_complete = get_record_encounter_complete(self)
    if record_encounter_complete then
        record_encounter_complete()
    end
    local ui = get_ui(self)
    local camera = get_camera(self)
    ui:show_banner(
        self.scene.title,
        self.config.outro_subtitle or "Gate released.",
        self.scene.accent_color,
        1.5
    )
    camera:shake(5, 0.16)
end

function EncounterController:update(dt, player)
    if self.beat and not self.beat_shown and player.x >= (self.beat.trigger_x or math.huge) then
        self.beat_shown = true
        local ui = get_ui(self)
        local camera = get_camera(self)
        ui:show_story_callout(
            self.beat.title or self.scene.title,
            self.beat.subtitle or "",
            self.scene.accent_color,
            self.beat.duration or 2.2
        )
        if self.beat.shake then
            camera:shake(self.beat.shake, 0.18)
        end
    end

    if not self.config or self.state == "disabled" or self.state == "complete" then
        return
    end

    if self.state == "idle" then
        if player.x >= (self.config.trigger_x or math.huge) then
            self:start()
        end
        return
    end

    if self.state == "arming" then
        self.wave_delay = self.wave_delay - dt
        if self.wave_delay <= 0 then
            self.wave_index = 1
            self:spawn_wave(self.config.waves[self.wave_index] or {})
            self.state = "active"
        end
        return
    end

    if self.state ~= "active" then
        return
    end

    local enemies = get_enemies(self)
    local live_enemies = #enemies.items > 0
    if live_enemies then
        return
    end

    local current_wave = self.config.waves[self.wave_index]
    if not current_wave then
        self:complete()
        return
    end

    if self.wave_index < #(self.config.waves or {}) then
        self.wave_delay = (current_wave.delay_after_clear or 0.45)
        self.wave_index = self.wave_index + 1
        self.state = "arming"
    else
        self:complete()
    end
end

function EncounterController:constrain_player(player)
    if not self:has_blockers() then
        return
    end

    local bounds = self.config.bounds
    local min_x = bounds.left or 0
    local max_x = (bounds.right or math.huge) - player.width

    if player.x < min_x then
        player.x = min_x
        player.vx = math.max(0, player.vx)
    elseif player.x > max_x then
        player.x = max_x
        player.vx = math.min(0, player.vx)
    end
end

function EncounterController:draw()
    if not self:has_blockers() then
        return
    end

    local bounds = self.config.bounds
    local top = 16
    local level = get_level(self)
    local bottom = level.map_height * 32 - 16

    local function draw_barrier(x, accent)
        love.graphics.setColor(accent[1], accent[2], accent[3], 0.16)
        love.graphics.rectangle("fill", x - 6, top, 12, bottom - top, 4, 4)
        love.graphics.setColor(1, 1, 1, 0.45)
        for y = top + 8, bottom - 8, 18 do
            love.graphics.line(x - 4, y, x + 4, y + 8)
        end
        love.graphics.setColor(accent[1], accent[2], accent[3], 0.26)
        love.graphics.rectangle("fill", x - 10, top - 4, 20, 6, 3, 3)
        love.graphics.rectangle("fill", x - 10, bottom - 2, 20, 6, 3, 3)
    end

    if bounds.left then
        draw_barrier(bounds.left, self.scene.accent_color)
    end
    if bounds.right then
        draw_barrier(bounds.right, self.scene.accent_color)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

return EncounterController
