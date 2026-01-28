-- enemies.lua
-- Manages all active enemies in a level.

local Enemy = require('enemy')
local Harpy = require('harpy')
local Utils = require('utils')

local TILE_SIZE = Utils.TILE_SIZE
local JUMP_FORCE = Utils.JUMP_FORCE

local Enemies = {}
Enemies.__index = Enemies

function Enemies:new()
    local self = setmetatable({}, Enemies)
    self.items = {}
    self.projectiles = {}
    self.shader = love.graphics.newShader("enemy.glsl")
    return self
end

function Enemies:spawn(x, y, type)
    local enemy
    if type == 'harpy' then
        enemy = Harpy:new(x,y)
    else -- Default to walker
        enemy = Enemy:new(x, y)
    end
    table.insert(self.items, enemy)
end

function Enemies:spawn_projectile(x, y, direction)
    local projectile = {
        x = x, y = y + 12, -- Fire from the middle
        width = 8, height = 4,
        vx = 600 * direction,
        lifetime = 3 -- Remove after 3 seconds
    }
    table.insert(self.projectiles, projectile)
end

function Enemies:update(dt, level, player)
    -- Update enemies
    for i = #self.items, 1, -1 do
        local enemy = self.items[i]
        enemy:update(dt, level, player)
        
        -- Player collision with enemy body
        if Utils.check_collision(player, enemy) and not player:is_invincible() then
            if player.vy > 0 and player.y + player.height < enemy.y + enemy.height then -- Stomp
                enemy:destroy()
                table.remove(self.items, i)
                player.vy = JUMP_FORCE * 0.6 -- Bounce
            else
                -- Player takes damage
                player:take_damage()
            end
        end
    end
    self:handle_level_collision(level)

    -- Update projectiles
    for i = #self.projectiles, 1, -1 do
        local p = self.projectiles[i]
        p.x = p.x + p.vx * dt
        p.lifetime = p.lifetime - dt

        if level:get_tile(p.x, p.y) > 0 or p.lifetime <= 0 then
            table.remove(self.projectiles, i)
        elseif Utils.check_collision(player, p) and not player:is_invincible() then
            player:take_damage()
            table.remove(self.projectiles, i)
        end
    end
end

function Enemies:draw()
    -- Draw enemies
    love.graphics.setShader(self.shader)
    for _, enemy in ipairs(self.items) do
        self.shader:send("time", love.timer.getTime())
        self.shader:send("base_color", enemy.color)
        enemy:draw()
    end
    love.graphics.setShader()

    -- Draw projectiles
    love.graphics.setColor(1, 0.2, 0.2)
    for _, p in ipairs(self.projectiles) do
        love.graphics.rectangle("fill", p.x, p.y, p.width, p.height)
    end
    love.graphics.setColor(1,1,1)
end

function Enemies:clear()
    self.items = {}
    self.projectiles = {}
end

function Enemies:handle_level_collision(level)
    for _, enemy in ipairs(self.items) do
        -- Basic ground detection
        local foot_x = enemy.x + enemy.width / 2
        local foot_y = enemy.y + enemy.height
        if level:get_tile(foot_x, foot_y + 1) > 0 then
            enemy.is_grounded = true
            enemy.vy = 0
            enemy.y = math.floor((foot_y + 1) / TILE_SIZE) * TILE_SIZE - enemy.height
        else
            enemy.is_grounded = false
        end
        
        -- Wall detection and reversal
        local next_x = enemy.x + (enemy.vx > 0 and enemy.width or 0) + (enemy.vx > 0 and 1 or -1)
        if level:get_tile(next_x, enemy.y + enemy.height/2) > 0 then
            enemy.vx = -enemy.vx
        end
    end
end

return Enemies 