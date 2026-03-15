-- fps_player.lua
-- First-person player controller: WASD movement, mouse look, shooting.

local FPSPlayer = {}
FPSPlayer.__index = FPSPlayer

local MOVE_SPEED = 3.0
local TURN_SPEED = 2.5
local MOUSE_SENSITIVITY = 0.003
local COLLISION_RADIUS = 0.25
local FIRE_RATE = 0.25
local FIRE_RANGE = 12

function FPSPlayer:new()
    local instance = setmetatable({}, self)
    instance.x = 2.5
    instance.y = 2.5
    instance.angle = 0
    instance.health = 5
    instance.max_health = 5
    instance.fire_cooldown = 0
    instance.weapon_state = "idle"
    instance.weapon_flash_timer = 0
    instance.damage_flash = 0
    instance.move_time = 0
    instance.is_dead = false
    instance.kills = 0
    instance.view_punch = 0
    instance.view_punch_velocity = 0
    instance.recoil_vx = 0
    instance.recoil_vy = 0
    return instance
end

function FPSPlayer:spawn(pos)
    self.x = pos.x
    self.y = pos.y
    self.angle = pos.angle or 0
    self.health = self.max_health
    self.is_dead = false
    self.fire_cooldown = 0
    self.weapon_state = "idle"
    self.kills = 0
end

function FPSPlayer:update(dt, map)
    if self.is_dead then return end

    self.fire_cooldown = math.max(0, self.fire_cooldown - dt)
    self.damage_flash = math.max(0, self.damage_flash - dt * 3)
    self.weapon_flash_timer = math.max(0, self.weapon_flash_timer - dt)

    if self.weapon_flash_timer <= 0 and self.weapon_state == "firing" then
        self.weapon_state = "idle"
    end

    -- View punch spring
    self.view_punch_velocity = self.view_punch_velocity - self.view_punch * 80 * dt
    self.view_punch_velocity = self.view_punch_velocity * (1 - 8 * dt)
    self.view_punch = self.view_punch + self.view_punch_velocity * dt

    -- Recoil push (pushes player backward)
    if math.abs(self.recoil_vx) > 0.01 or math.abs(self.recoil_vy) > 0.01 then
        local rx = self.recoil_vx * dt
        local ry = self.recoil_vy * dt
        if not map:is_solid(self.x + rx + (rx > 0 and COLLISION_RADIUS or -COLLISION_RADIUS), self.y) then
            self.x = self.x + rx
        end
        if not map:is_solid(self.x, self.y + ry + (ry > 0 and COLLISION_RADIUS or -COLLISION_RADIUS)) then
            self.y = self.y + ry
        end
        self.recoil_vx = self.recoil_vx * (1 - 10 * dt)
        self.recoil_vy = self.recoil_vy * (1 - 10 * dt)
    end

    -- Mouse look
    -- (Mouse delta handled in mousemoved callback)

    -- Keyboard turn (fallback)
    if love.keyboard.isDown("left") then
        self.angle = self.angle - TURN_SPEED * dt
    end
    if love.keyboard.isDown("right") then
        self.angle = self.angle + TURN_SPEED * dt
    end

    -- Movement
    local move_x, move_y = 0, 0
    local cos_a = math.cos(self.angle)
    local sin_a = math.sin(self.angle)
    local moving = false

    if love.keyboard.isDown("w", "up") then
        move_x = move_x + cos_a
        move_y = move_y + sin_a
        moving = true
    end
    if love.keyboard.isDown("s", "down") then
        move_x = move_x - cos_a
        move_y = move_y - sin_a
        moving = true
    end
    if love.keyboard.isDown("a") then
        move_x = move_x + sin_a
        move_y = move_y - cos_a
        moving = true
    end
    if love.keyboard.isDown("d") then
        move_x = move_x - sin_a
        move_y = move_y + cos_a
        moving = true
    end

    if moving then
        self.move_time = self.move_time + dt
    end

    -- Normalize
    local len = math.sqrt(move_x * move_x + move_y * move_y)
    if len > 0 then
        move_x = move_x / len * MOVE_SPEED * dt
        move_y = move_y / len * MOVE_SPEED * dt
    end

    -- Collision with walls (slide along walls)
    local new_x = self.x + move_x
    local new_y = self.y + move_y
    local r = COLLISION_RADIUS

    if not map:is_solid(new_x + r, self.y) and not map:is_solid(new_x - r, self.y) then
        self.x = new_x
    end
    if not map:is_solid(self.x, new_y + r) and not map:is_solid(self.x, new_y - r) then
        self.y = new_y
    end

    -- Pickup collision
    for _, sprite in ipairs(map.sprites) do
        if sprite.active and (sprite.type == "pickup_health" or sprite.type == "pickup_key") then
            local dx = sprite.x - self.x
            local dy = sprite.y - self.y
            if dx * dx + dy * dy < 0.5 then
                sprite.active = false
                if sprite.type == "pickup_health" then
                    self.health = math.min(self.max_health, self.health + 1)
                end
            end
        end
    end
end

function FPSPlayer:mousemoved(dx)
    if self.is_dead then return end
    self.angle = self.angle + dx * MOUSE_SENSITIVITY
end

function FPSPlayer:fire(map, sfx)
    if self.is_dead or self.fire_cooldown > 0 then return end

    self.fire_cooldown = FIRE_RATE
    self.weapon_state = "firing"
    self.weapon_flash_timer = 0.08

    -- View punch and recoil
    self.view_punch_velocity = self.view_punch_velocity - 4
    local cos_a = math.cos(self.angle)
    local sin_a = math.sin(self.angle)
    self.recoil_vx = -cos_a * 2.5
    self.recoil_vy = -sin_a * 2.5

    if sfx then sfx:play("shoot") end

    -- Hitscan: check if we hit a sprite

    local best_dist = FIRE_RANGE
    local best_target = nil

    for _, sprite in ipairs(map.sprites) do
        if sprite.active and (sprite.type == "enemy" or sprite.type == "demon") then
            local dx = sprite.x - self.x
            local dy = sprite.y - self.y
            local dist = math.sqrt(dx * dx + dy * dy)

            if dist < best_dist then
                -- Check if sprite is roughly in front of us
                local sprite_angle = math.atan2(dy, dx) - self.angle
                while sprite_angle > math.pi do sprite_angle = sprite_angle - math.pi * 2 end
                while sprite_angle < -math.pi do sprite_angle = sprite_angle + math.pi * 2 end

                -- Hit width based on distance
                local hit_angle = math.atan2(0.4, dist)
                if math.abs(sprite_angle) < hit_angle then
                    best_dist = dist
                    best_target = sprite
                end
            end
        end
    end

    if best_target then
        best_target.health = best_target.health - 1
        if best_target.health <= 0 then
            best_target.active = false
            self.kills = self.kills + 1
            if sfx then sfx:play("stomp") end
        else
            if sfx then sfx:play("hit_enemy") end
        end
        return true
    end
    return false
end

function FPSPlayer:get_view_angle()
    return self.angle + self.view_punch * 0.03
end

function FPSPlayer:take_damage(amount, sfx)
    if self.is_dead then return end
    self.health = self.health - (amount or 1)
    self.damage_flash = 1
    if sfx then sfx:play("hit_enemy") end
    if self.health <= 0 then
        self.is_dead = true
        if sfx then sfx:play("death") end
    end
end

return FPSPlayer
