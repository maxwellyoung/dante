-- harpy.lua
-- Defines a flying, projectile-firing enemy.

local Palette = require("infernal_ascent_palette")
local Harpy = {}
Harpy.__index = Harpy

local function get_sfx(self)
    return self.services and self.services.sfx or g_sfx
end

local function get_effects(self)
    return self.services and self.services.effects or g_effects
end

local function get_enemies(self)
    return self.services and self.services.enemies or g_enemies
end

function Harpy:new(x, y, services)
    local class = self or Harpy
    local instance = setmetatable({}, class)
    instance.services = services
    instance.x = x
    instance.y = y
    instance.start_y = y
    instance.width = 32
    instance.height = 28
    instance.vx = 0
    instance.vy = 0
    instance.is_active = true
    instance.bob_speed = 2
    instance.bob_height = 16
    instance.fire_cooldown = 0
    instance.fire_rate = 1.6
    instance.telegraph_timer = 0
    instance.telegraph_duration = 0.45
    instance.telegraph_target = nil
    instance.color = Palette.enemy.harpy_body
    return instance
end

function Harpy:update(dt, level, player)
    if not self.is_active then
        return
    end

    self.y = self.start_y + math.sin(love.timer.getTime() * self.bob_speed) * self.bob_height

    if self.fire_cooldown > 0 then
        self.fire_cooldown = self.fire_cooldown - dt
    end

    -- Telegraph state: charging up before firing
    if self.telegraph_timer > 0 then
        self.telegraph_timer = self.telegraph_timer - dt
        if self.telegraph_timer <= 0 and self.telegraph_target then
            self:fire(self.telegraph_target)
            self.fire_cooldown = self.fire_rate
            self.telegraph_target = nil
        end
        return
    end

    if self.fire_cooldown <= 0 and self:has_line_of_sight(player, level) then
        -- Begin telegraph instead of firing immediately
        self.telegraph_timer = self.telegraph_duration
        self.telegraph_target = player
        local sfx = get_sfx(self)
        if sfx then
            sfx:play("harpy_charge")
        end
    end
end

function Harpy:has_line_of_sight(player, level)
    if not player then
        return false
    end

    local dx = math.abs(player.x - self.x)
    if dx > 300 then
        return false
    end

    local dy = math.abs(player.y - self.y)
    if dy > 64 then
        return false
    end

    local start_x = math.min(self.x + self.width / 2, player.x + player.width / 2)
    local end_x = math.max(self.x + self.width / 2, player.x + player.width / 2)
    local check_y = (self.y + player.y) / 2

    for x = start_x, end_x, 16 do
        if level:get_tile(x, check_y) > 0 then
            return false
        end
        if level:get_tile(x, self.y + self.height / 2) > 0 then
            return false
        end
    end

    return true
end

function Harpy:fire(player)
    local sfx = get_sfx(self)
    local enemies = get_enemies(self)
    if sfx then
        sfx:play("shriek")
    end
    local projectile_dir = (player.x < self.x) and -1 or 1
    if enemies then
        enemies:spawn_projectile(self.x, self.y, projectile_dir)
    end
end

function Harpy:draw()
    if not self.is_active then
        return
    end

    -- Telegraph warning: pulsing red glow and aim line
    if self.telegraph_timer > 0 then
        local pulse = 0.5 + 0.5 * math.sin(love.timer.getTime() * 24)
        local cx = self.x + self.width / 2
        local cy = self.y + self.height / 2
        -- Warning glow
        love.graphics.setColor(Palette.enemy.telegraph[1], Palette.enemy.telegraph[2], Palette.enemy.telegraph[3], 0.3 * pulse)
        love.graphics.circle("fill", cx, cy, 22)
        -- Aim line toward target
        if self.telegraph_target then
            local tx = self.telegraph_target.x + self.telegraph_target.width / 2
            local ty = self.telegraph_target.y + self.telegraph_target.height / 2
            local dx = tx - cx
            local dy = ty - cy
            local dist = math.sqrt(dx * dx + dy * dy)
            if dist > 0 then
                local nx, ny = dx / dist, dy / dist
                love.graphics.setColor(Palette.enemy.telegraph_line[1], Palette.enemy.telegraph_line[2], Palette.enemy.telegraph_line[3], 0.4 * pulse)
                love.graphics.setLineWidth(2)
                love.graphics.line(cx + nx * 16, cy + ny * 16, cx + nx * 60, cy + ny * 60)
                love.graphics.setLineWidth(1)
            end
        end
    end

    love.graphics.setColor(self.color[1], self.color[2], self.color[3], 1)
    love.graphics.rectangle("fill", self.x + 4, self.y + 8, self.width - 8, self.height - 8, 4, 4)
    love.graphics.setColor(Palette.enemy.harpy_wing[1], Palette.enemy.harpy_wing[2], Palette.enemy.harpy_wing[3], 1)
    love.graphics.rectangle("fill", self.x, self.y + 12, 6, 10, 2, 2)
    love.graphics.rectangle("fill", self.x + self.width - 6, self.y + 12, 6, 10, 2, 2)

    -- Eyes flash red during telegraph
    if self.telegraph_timer > 0 then
        local flash = 0.6 + 0.4 * math.sin(love.timer.getTime() * 20)
        love.graphics.setColor(1, 0.2 * flash, 0.1 * flash, 1)
    else
        love.graphics.setColor(Palette.enemy.harpy_face[1], Palette.enemy.harpy_face[2], Palette.enemy.harpy_face[3], 1)
    end
    love.graphics.rectangle("fill", self.x + 10, self.y + 10, self.width - 20, 8, 2, 2)
    love.graphics.setColor(1, 1, 1, 1)
end

function Harpy:destroy()
    self.is_active = false
    local sfx = get_sfx(self)
    local effects = get_effects(self)
    if sfx then
        sfx:play("stomp")
    end
    if effects then
        effects:stomp(self.x + self.width / 2, self.y + self.height / 2)
    end
end

return Harpy
