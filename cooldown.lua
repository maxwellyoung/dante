-- cooldown.lua
-- A simple timer object for managing cooldowns.

local Cooldown = {}
Cooldown.__index = Cooldown

function Cooldown:new(duration)
    local self = setmetatable({}, Cooldown)
    self.duration = duration
    self.timer = 0
    return self
end

function Cooldown:update(dt)
    if self.timer > 0 then
        self.timer = math.max(0, self.timer - dt)
    end
end

function Cooldown:is_ready()
    return self.timer == 0
end

function Cooldown:use()
    self.timer = self.duration
end

function Cooldown:set_duration(duration)
    self.duration = duration
end

return Cooldown 