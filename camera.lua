-- camera.lua
-- A simple camera that handles letterboxing to maintain a fixed aspect ratio.

local Camera = {}
Camera.__index = Camera

function Camera:new(native_w, native_h)
    local self = setmetatable({}, Camera)
    self.native_w = native_w
    self.native_h = native_h
    self:resize(love.graphics.getWidth(), love.graphics.getHeight())
    return self
end

function Camera:resize(w, h)
    local native_aspect = self.native_w / self.native_h
    local window_aspect = w / h

    if window_aspect > native_aspect then
        -- Window is wider than the game (letterbox)
        self.scale = h / self.native_h
        self.x = (w - self.native_w * self.scale) / 2
        self.y = 0
    else
        -- Window is taller than the game (pillarbox)
        self.scale = w / self.native_w
        self.x = 0
        self.y = (h - self.native_h * self.scale) / 2
    end
end

function Camera:attach()
    love.graphics.push()
    love.graphics.translate(self.x, self.y)
    love.graphics.scale(self.scale, self.scale)
end

function Camera:detach()
    love.graphics.pop()
end

return Camera 