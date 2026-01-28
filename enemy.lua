-- enemy.lua
-- Defines the behavior for a single enemy type.

local Enemy = {}
Enemy.__index = Enemy

function Enemy:new(x, y)
    local self = setmetatable({}, Enemy)
    self.x = x
    self.y = y
    self.width = 32
    self.height = 32
    self.vx = -50 -- Start by moving left
    self.vy = 0
    self.is_active = true
    self.is_grounded = false
    self.color = { math.random(), math.random() * 0.5, math.random() * 0.8 }
    return self
end

function Enemy:update(dt)
    if not self.is_active then return end

    -- Simple gravity
    if not self.is_grounded then
        self.vy = self.vy + 1200 * dt
    end

    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt
end

function Enemy:draw()
    if not self.is_active then return end
    
    -- The new shader handles drawing, but we still need this empty rectangle
    -- to define the area for the shader to operate on.
    love.graphics.setColor(1,1,1)
    love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
end

function Enemy:destroy()
    self.is_active = false
    -- Trigger death effects
    g_sfx:play('stomp')
    g_effects:stomp(self.x + self.width / 2, self.y + self.height / 2)
end

return Enemy 