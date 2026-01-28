-- harpy.lua
-- Defines a flying, projectile-firing enemy.

local Harpy = {}
Harpy.__index = Harpy

function Harpy:new(x, y)
    local self = setmetatable({}, Harpy)
    self.x = x
    self.y = y
    self.start_y = y
    self.width = 32
    self.height = 32
    self.vx = 0
    self.vy = 0
    self.is_active = true
    self.bob_speed = 2
    self.bob_height = 16
    self.fire_cooldown = 0
    self.fire_rate = 2 -- Fires every 2 seconds
    self.color = {0.8, 0.8, 0.2} -- A sickly yellow
    return self
end

function Harpy:update(dt, level, player)
    if not self.is_active then return end

    -- Bobbing motion
    self.y = self.start_y + math.sin(love.timer.getTime() * self.bob_speed) * self.bob_height

    -- Cooldown timer
    if self.fire_cooldown > 0 then
        self.fire_cooldown = self.fire_cooldown - dt
    end

    -- Check for line of sight and fire
    if self.fire_cooldown <= 0 and self:has_line_of_sight(player, level) then
        self:fire(player)
        self.fire_cooldown = self.fire_rate
    end
end

function Harpy:has_line_of_sight(player, level)
    if not player then return false end

    -- Check horizontal distance - max range of 300 pixels
    local dx = math.abs(player.x - self.x)
    if dx > 300 then return false end

    -- Check vertical distance - more generous range for flying enemy
    local dy = math.abs(player.y - self.y)
    if dy > 64 then return false end

    local start_x = math.min(self.x + self.width/2, player.x + player.width/2)
    local end_x = math.max(self.x + self.width/2, player.x + player.width/2)
    local check_y = (self.y + player.y) / 2 -- Check at midpoint height

    -- Ray cast from harpy to player, checking for walls
    for x = start_x, end_x, 16 do
        if level:get_tile(x, check_y) > 0 then
            return false -- Wall is in the way
        end
        -- Also check at harpy's height
        if level:get_tile(x, self.y + self.height/2) > 0 then
            return false
        end
    end

    return true
end


function Harpy:fire(player)
    g_sfx:play("shriek")
    local projectile_dir = (player.x < self.x) and -1 or 1
    g_enemies:spawn_projectile(self.x, self.y, projectile_dir)
end


function Harpy:draw()
    if not self.is_active then return end
    
    -- Placeholder draw. The shader in enemies.lua will style it.
    love.graphics.setColor(1,1,1)
    love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
end

function Harpy:destroy()
    self.is_active = false
    g_sfx:play('stomp')
    g_effects:stomp(self.x + self.width / 2, self.y + self.height / 2)
end

return Harpy 