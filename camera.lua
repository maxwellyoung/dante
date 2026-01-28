-- camera.lua
-- A simple camera that follows a target within a low-resolution canvas.

Camera = {}
Camera.__index = Camera

function Camera:new(player)
    local self = setmetatable({}, Camera)
    self.player = player
    self.x = 0
    self.y = 0
    self.scale = 1 -- Scale is now handled by the main render pipeline
    self.shake_duration = 0
    self.shake_intensity = 0
    return self
end

function Camera:update(dt)
    -- Camera Shake Logic
    if self.shake_duration > 0 then
        self.shake_duration = self.shake_duration - dt
        if self.shake_duration <= 0 then
            self.shake_intensity = 0
        end
    end

    -- Smoothly follow the player, centering on the native resolution
    local target_x = self.player.x - g_native_width / 2
    local target_y = self.player.y - g_native_height / 2

    -- Smooth interpolation
    self.x = self.x + (target_x - self.x) * 5 * dt
    self.y = self.y + (target_y - self.y) * 5 * dt

    -- Clamp to level bounds if level exists
    if g_level then
        local level_width = g_level.map_width * 32
        local level_height = g_level.map_height * 32
        self.x = math.max(0, math.min(self.x, level_width - g_native_width))
        self.y = math.max(0, math.min(self.y, level_height - g_native_height))
    end
end

function Camera:attach()
    love.graphics.push()
    love.graphics.scale(self.scale, self.scale)
    local shake_x = (math.random() * 2 - 1) * self.shake_intensity
    local shake_y = (math.random() * 2 - 1) * self.shake_intensity
    love.graphics.translate(-math.floor(self.x + shake_x), -math.floor(self.y + shake_y))
end

function Camera:detach()
    love.graphics.pop()
end

function Camera:shake(intensity, duration)
    self.shake_intensity = intensity
    self.shake_duration = duration
end

function Camera:reset()
    if self.player then
        self.x = self.player.x - g_native_width / 2
        self.y = self.player.y - g_native_height / 2
    end
    self.shake_intensity = 0
    self.shake_duration = 0
end

function Camera:to_world(x, y)
    -- No scaling needed here anymore
    local world_x = x + self.x
    local world_y = y + self.y
    return world_x, world_y
end

function Camera:to_screen(world_x, world_y)
    -- No scaling needed here anymore
    local screen_x = world_x - self.x
    local screen_y = world_y - self.y
    return screen_x, screen_y
end

return Camera 