-- projectiles.lua
-- Manages player-fired tracers and immediate hit resolution.

local Utils = require("utils")

local Projectiles = {}
Projectiles.__index = Projectiles

local MAX_RANGE = 560
local STEP_SIZE = 8

function Projectiles:new()
    local class = self or Projectiles
    local instance = setmetatable({}, class)
    instance.items = {}
    return instance
end

function Projectiles:add_tracer(x1, y1, x2, y2, hit_kind)
    local color = { 1, 0.88, 0.45, 1 }
    if hit_kind == "wall" then
        color = { 1, 0.72, 0.42, 1 }
    elseif hit_kind == "enemy" then
        color = { 1, 0.94, 0.72, 1 }
    end

    table.insert(self.items, {
        x1 = x1,
        y1 = y1,
        x2 = x2,
        y2 = y2,
        lifetime = 0.07,
        max_lifetime = 0.07,
        color = color,
    })
end

function Projectiles:spawn(x, y, target_x, target_y)
    local dx = target_x - x
    local dy = target_y - y
    local distance = math.max(1, math.sqrt(dx * dx + dy * dy))
    local dir_x = dx / distance
    local dir_y = dy / distance
    local hit_x = x + dir_x * MAX_RANGE
    local hit_y = y + dir_y * MAX_RANGE
    local hit_kind = "miss"

    for travel = STEP_SIZE, MAX_RANGE, STEP_SIZE do
        local sample_x = x + dir_x * travel
        local sample_y = y + dir_y * travel

        if g_level:get_tile(sample_x, sample_y) == 1 then
            hit_x = sample_x
            hit_y = sample_y
            hit_kind = "wall"
            g_sfx:play("hit_wall")
            g_effects:spawn("sparks", hit_x, hit_y)
            break
        end

        local probe = {
            x = sample_x - 2,
            y = sample_y - 2,
            width = 4,
            height = 4,
        }
        for enemy_index = #g_enemies.items, 1, -1 do
            local enemy = g_enemies.items[enemy_index]
            if Utils.check_collision(probe, enemy) then
                hit_x = sample_x
                hit_y = sample_y
                hit_kind = "enemy"
                g_sfx:play("hit_enemy")
                g_effects:spawn("blood", hit_x, hit_y)
                g_camera:shake(5, 0.12)
                if g_run_stats then
                    g_run_stats.shots_hit = g_run_stats.shots_hit + 1
                    g_run_stats.room_shots_hit = g_run_stats.room_shots_hit + 1
                end
                enemy:destroy()
                table.remove(g_enemies.items, enemy_index)
                break
            end
        end

        if hit_kind == "enemy" then
            break
        end
    end

    self:add_tracer(x, y, hit_x, hit_y, hit_kind)
    g_sfx:play("shoot")
end

function Projectiles:update(dt, _level, _enemies)
    for index = #self.items, 1, -1 do
        local projectile = self.items[index]
        projectile.lifetime = projectile.lifetime - dt
        if projectile.lifetime <= 0 then
            table.remove(self.items, index)
        end
    end
end

function Projectiles:draw()
    for _, projectile in ipairs(self.items) do
        local alpha = math.max(0, projectile.lifetime / projectile.max_lifetime)
        love.graphics.setColor(projectile.color[1], projectile.color[2], projectile.color[3], alpha)
        love.graphics.setLineWidth(2)
        love.graphics.line(projectile.x1, projectile.y1, projectile.x2, projectile.y2)
        love.graphics.setColor(1, 1, 1, alpha * 0.9)
        love.graphics.circle("fill", projectile.x2, projectile.y2, 2)
    end
    love.graphics.setLineWidth(1)
    love.graphics.setColor(1, 1, 1, 1)
end

return Projectiles
