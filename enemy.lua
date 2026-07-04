-- enemy.lua
-- Defines the behavior for a single enemy type.

local Palette = require("infernal_ascent_palette")
local Enemy = {}
Enemy.__index = Enemy

local function get_sfx(self)
    return self.services and self.services.sfx or g_sfx
end

local function get_effects(self)
    return self.services and self.services.effects or g_effects
end

function Enemy:new(x, y, services)
    local class = self or Enemy
    local instance = setmetatable({}, class)
    instance.services = services
    instance.x = x
    instance.y = y
    instance.width = 32
    instance.height = 32
    instance.vx = -58
    instance.vy = 0
    instance.is_active = true
    instance.is_grounded = false
    instance.color = Palette.enemy.walker_body
    return instance
end

function Enemy:update(dt)
    if not self.is_active then
        return
    end

    if not self.is_grounded then
        self.vy = self.vy + 1200 * dt
    end

    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt
end

function Enemy:draw()
    if not self.is_active then
        return
    end

    love.graphics.setColor(self.color[1], self.color[2], self.color[3], 1)
    love.graphics.rectangle("fill", self.x, self.y + 6, self.width, self.height - 6, 4, 4)
    love.graphics.setColor(Palette.enemy.walker_shadow[1], Palette.enemy.walker_shadow[2], Palette.enemy.walker_shadow[3], 1)
    love.graphics.rectangle("fill", self.x + 5, self.y + 12, self.width - 10, 6, 2, 2)
    love.graphics.setColor(Palette.enemy.walker_face[1], Palette.enemy.walker_face[2], Palette.enemy.walker_face[3], 1)
    love.graphics.rectangle("fill", self.x + 8, self.y + 7, self.width - 16, 8, 2, 2)
    love.graphics.setColor(1, 1, 1, 1)
end

function Enemy:destroy()
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

return Enemy
