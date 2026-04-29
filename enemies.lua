-- enemies.lua
-- Manages all active enemies in a level.

local Enemy = require("enemy")
local Harpy = require("harpy")
local Palette = require("infernal_ascent_palette")
local Utils = require("utils")

local TILE_SIZE = Utils.TILE_SIZE
local JUMP_FORCE = Utils.JUMP_FORCE

local Enemies = {}
Enemies.__index = Enemies

local function get_effects(self)
    return self.services and self.services.effects or g_effects
end

local function get_trigger_hitstop(self)
    return self.services and self.services.trigger_hitstop or g_trigger_hitstop
end

local function get_camera(self)
    return self.services and self.services.camera or g_camera
end

function Enemies:new(services)
    local class = self or Enemies
    local instance = setmetatable({}, class)
    instance.services = services
    instance.items = {}
    instance.projectiles = {}
    return instance
end

function Enemies:spawn(x, y, enemy_type)
    local enemy
    if enemy_type == "harpy" then
        enemy = Harpy:new(x, y, self.services)
    else
        enemy = Enemy:new(x, y, self.services)
    end
    table.insert(self.items, enemy)
end

function Enemies:spawn_projectile(x, y, direction)
    local projectile = {
        x = x,
        y = y + 12,
        width = 10,
        height = 6,
        vx = 520 * direction,
        lifetime = 2.5,
    }
    table.insert(self.projectiles, projectile)
end

function Enemies:update(dt, level, player)
    for i = #self.items, 1, -1 do
        local enemy = self.items[i]
        enemy:update(dt, level, player)

        if Utils.check_collision(player, enemy) and not player:is_invincible() then
            if player.vy > 0 and player.y + player.height < enemy.y + enemy.height then
                local ex = enemy.x + enemy.width / 2
                local ey = enemy.y + enemy.height / 2
                enemy:destroy()
                table.remove(self.items, i)
                player.vy = JUMP_FORCE * 0.58
                -- Stomp juice: ring effect, hitstop, camera shake, sound
                local sfx = self.services and self.services.sfx or g_sfx
                local effects = get_effects(self)
                local trigger_hitstop = get_trigger_hitstop(self)
                local camera = get_camera(self)
                if sfx then
                    sfx:play("stomp_bounce")
                end
                if effects then
                    effects:spawn("stomp_ring", ex, ey)
                end
                if trigger_hitstop then
                    trigger_hitstop(0.06)
                end
                if camera then
                    camera:shake(5, 0.12)
                end
            else
                player:take_damage()
            end
        end
    end
    self:handle_level_collision(level)

    for i = #self.projectiles, 1, -1 do
        local projectile = self.projectiles[i]
        projectile.x = projectile.x + projectile.vx * dt
        projectile.lifetime = projectile.lifetime - dt

        if level:get_tile(projectile.x, projectile.y) > 0 or projectile.lifetime <= 0 then
            table.remove(self.projectiles, i)
        elseif Utils.check_collision(player, projectile) and not player:is_invincible() then
            local effects = get_effects(self)
            player:take_damage()
            if effects then
                effects:spawn("sparks", projectile.x + projectile.width / 2, projectile.y + projectile.height / 2)
            end
            table.remove(self.projectiles, i)
        end
    end
end

function Enemies:draw()
    for _, enemy in ipairs(self.items) do
        enemy:draw()
    end

    love.graphics.setColor(Palette.enemy.projectile[1], Palette.enemy.projectile[2], Palette.enemy.projectile[3], 1)
    for _, projectile in ipairs(self.projectiles) do
        love.graphics.rectangle(
            "fill",
            projectile.x,
            projectile.y,
            projectile.width,
            projectile.height,
            2,
            2
        )
    end
    love.graphics.setColor(1, 1, 1, 1)
end

function Enemies:clear()
    self.items = {}
    self.projectiles = {}
end

function Enemies:handle_level_collision(level)
    for _, enemy in ipairs(self.items) do
        local foot_x = enemy.x + enemy.width / 2
        local foot_y = enemy.y + enemy.height
        if level:get_tile(foot_x, foot_y + 1) > 0 then
            enemy.is_grounded = true
            enemy.vy = 0
            enemy.y = math.floor((foot_y + 1) / TILE_SIZE) * TILE_SIZE - enemy.height
        else
            enemy.is_grounded = false
        end

        local next_x = enemy.x + (enemy.vx > 0 and enemy.width or 0) + (enemy.vx > 0 and 1 or -1)
        if level:get_tile(next_x, enemy.y + enemy.height / 2) > 0 then
            enemy.vx = -enemy.vx
        end
    end
end

return Enemies
