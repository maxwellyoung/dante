-- harpy.lua
-- Defines a flying, projectile-firing enemy.

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
    instance.color = { 0.78, 0.76, 0.32 }
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

    if self.fire_cooldown <= 0 and self:has_line_of_sight(player, level) then
        self:fire(player)
        self.fire_cooldown = self.fire_rate
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

    love.graphics.setColor(self.color[1], self.color[2], self.color[3], 1)
    love.graphics.rectangle("fill", self.x + 4, self.y + 8, self.width - 8, self.height - 8, 4, 4)
    love.graphics.setColor(0.26, 0.2, 0.08, 1)
    love.graphics.rectangle("fill", self.x, self.y + 12, 6, 10, 2, 2)
    love.graphics.rectangle("fill", self.x + self.width - 6, self.y + 12, 6, 10, 2, 2)
    love.graphics.setColor(1, 0.95, 0.78, 1)
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
