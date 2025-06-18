-- camera.lua
-- A simple camera that handles letterboxing and follows a target.

local Camera = {}
Camera.__index = Camera

-- Camera settings
local LERP_RATE = 4 -- How quickly the camera follows the player (higher is faster)
local NATIVE_WIDTH = 800
local NATIVE_HEIGHT = 600

function Camera:new(target)
    local self = setmetatable({}, Camera)
    self.target = target
    self.x = 0
    self.y = 0

    self.scale = 1
    self.offset_x = 0
    self.offset_y = 0
    
    if self.target then
        self.x = self.target.x - NATIVE_WIDTH / 2
        self.y = self.target.y - NATIVE_HEIGHT / 2
    end
    
    self:resize(love.graphics.getWidth(), love.graphics.getHeight())
    
    return self
end

function Camera:update(dt)
    if not self.target then return end

    -- Smoothly interpolate camera position towards the target
    local target_x = self.target.x - NATIVE_WIDTH / 2
    local target_y = self.target.y - NATIVE_HEIGHT / 2
    
    self.x = self.x + (target_x - self.x) * LERP_RATE * dt
    self.y = self.y + (target_y - self.y) * LERP_RATE * dt
end

function Camera:resize(w, h)
    local native_aspect = NATIVE_WIDTH / NATIVE_HEIGHT
    local window_aspect = w / h

    if window_aspect > native_aspect then
        -- Window is wider than the game (letterbox)
        self.scale = h / NATIVE_HEIGHT
        self.offset_x = (w - NATIVE_WIDTH * self.scale) / 2
        self.offset_y = 0
    else
        -- Window is taller than the game (pillarbox)
        self.scale = w / NATIVE_WIDTH
        self.offset_x = 0
        self.offset_y = (h - NATIVE_HEIGHT * self.scale) / 2
    end
end

function Camera:attach()
    love.graphics.push()
    love.graphics.translate(self.offset_x, self.offset_y)
    love.graphics.scale(self.scale, self.scale)
    love.graphics.translate(-math.floor(self.x), -math.floor(self.y))
end

function Camera:detach()
    love.graphics.pop()
end

return Camera 