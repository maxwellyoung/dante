-- projectiles.lua
-- Manages all player-fired projectiles.

local Utils = require('utils')

local Projectiles = {}
Projectiles.__index = Projectiles

function Projectiles:new()
    local self = setmetatable({}, Projectiles)
    self.items = {}
    return self
end

function Projectiles:spawn(x, y, vx, vy)
    local proj = {
        x = x,
        y = y,
        width = 8,
        height = 8,
        vx = vx,
        vy = vy,
        lifetime = 2, -- a projectile will exist for 2 seconds
        color = {1, 1, 0.5, 1} -- Bright yellow
    }
    table.insert(self.items, proj)
    g_sfx:play('shoot')
end

function Projectiles:update(dt, level, enemies)
    for i = #self.items, 1, -1 do
        local p = self.items[i]
        p.x = p.x + p.vx * dt
        p.y = p.y + p.vy * dt
        p.lifetime = p.lifetime - dt

        if p.lifetime <= 0 then
            table.remove(self.items, i)
        -- Check for collision with the level
        elseif level:get_tile(p.x, p.y) > 0 then
            g_sfx:play('hit_wall')
            g_effects:spawn('sparks', p.x, p.y, 'up')
            table.remove(self.items, i)
        else
            -- Check for collision with enemies
            for j = #enemies.items, 1, -1 do
                local enemy = enemies.items[j]
                if Utils.check_collision(p, enemy) then
                    g_sfx:play('hit_enemy')
                    g_effects:spawn('blood', p.x, p.y, 'up')
                    g_camera:shake(8, 0.2) -- Add screen shake!
                    
                    enemy:destroy()
                    table.remove(enemies.items, j)
                    table.remove(self.items, i)
                    break 
                end
            end
        end
    end
end

function Projectiles:draw()
    for _, p in ipairs(self.items) do
        love.graphics.setColor(unpack(p.color))
        love.graphics.rectangle("fill", p.x, p.y, p.width, p.height)
    end
    love.graphics.setColor(1, 1, 1, 1)
end

return Projectiles 