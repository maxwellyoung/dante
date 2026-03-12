-- enemy.lua
-- Defines the behavior for a single enemy type.

local Enemy = {}
Enemy.__index = Enemy

function Enemy:new(x, y)
    local class = self or Enemy
    local instance = setmetatable({}, class)
    instance.x = x
    instance.y = y
    instance.width = 32
    instance.height = 32
    instance.vx = -58
    instance.vy = 0
    instance.is_active = true
    instance.is_grounded = false
    instance.color = { 0.84, 0.28, 0.24 }
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
    love.graphics.setColor(0.18, 0.07, 0.07, 1)
    love.graphics.rectangle("fill", self.x + 5, self.y + 12, self.width - 10, 6, 2, 2)
    love.graphics.setColor(1, 0.88, 0.74, 1)
    love.graphics.rectangle("fill", self.x + 8, self.y + 7, self.width - 16, 8, 2, 2)
    love.graphics.setColor(1, 1, 1, 1)
end

function Enemy:destroy()
    self.is_active = false
    g_sfx:play("stomp")
    g_effects:stomp(self.x + self.width / 2, self.y + self.height / 2)
end

return Enemy
