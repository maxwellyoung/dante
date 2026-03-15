-- fps_player.lua
-- FPS player: WASD, mouse look, sprint, wall-jump, weapon system.

local FPSPlayer = {}
FPSPlayer.__index = FPSPlayer

local MOVE_SPEED = 3.2
local SPRINT_SPEED = 5.5
local TURN_SPEED = 2.5
local MOUSE_SENSITIVITY = 0.003
local COLLISION_RADIUS = 0.25
local WALL_JUMP_PUSH = 4.0
local WALL_JUMP_COOLDOWN = 0.3

-- Weapon definitions
local WEAPONS = {
    pistol = {
        name = "PISTOL",
        fire_rate = 0.3,
        damage = 1,
        range = 14,
        spread = 0,
        ricochet = 0,
        recoil = 2.0,
        color = { 0.9, 0.85, 0.4 },
        auto = false,
    },
    shotgun = {
        name = "SHOTGUN",
        fire_rate = 0.7,
        damage = 2,
        range = 8,
        spread = 0.12,
        ricochet = 0,
        recoil = 5.0,
        pellets = 5,
        color = { 1, 0.7, 0.3 },
        auto = false,
    },
    bouncer = {
        name = "BOUNCER",
        fire_rate = 0.4,
        damage = 1,
        range = 16,
        spread = 0,
        ricochet = 2,
        recoil = 1.5,
        color = { 0.4, 0.8, 1 },
        auto = false,
    },
    auto_rifle = {
        name = "AUTO",
        fire_rate = 0.1,
        damage = 1,
        range = 12,
        spread = 0.04,
        ricochet = 0,
        recoil = 1.0,
        color = { 1, 0.5, 0.3 },
        auto = true,
    },
}

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

    -- Sprint
    instance.is_sprinting = false
    instance.sprint_stamina = 1
    instance.sprint_regen_delay = 0

    -- Wall-jump
    instance.wall_jump_timer = 0
    instance.wall_push_vx = 0
    instance.wall_push_vy = 0

    -- Weapons
    instance.weapons = { "pistol" }
    instance.current_weapon = 1
    instance.holding_fire = false

    return instance
end

function FPSPlayer:get_weapon()
    local name = self.weapons[self.current_weapon]
    return WEAPONS[name] or WEAPONS.pistol, name
end

function FPSPlayer:give_weapon(name)
    for _, w in ipairs(self.weapons) do
        if w == name then return end
    end
    table.insert(self.weapons, name)
end

function FPSPlayer:next_weapon()
    self.current_weapon = self.current_weapon % #self.weapons + 1
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
    self.view_punch = 0
    self.view_punch_velocity = 0
    self.recoil_vx = 0
    self.recoil_vy = 0
    self.sprint_stamina = 1
    self.wall_jump_timer = 0
    self.wall_push_vx = 0
    self.wall_push_vy = 0
end

function FPSPlayer:update(dt, map)
    if self.is_dead then return end

    self.fire_cooldown = math.max(0, self.fire_cooldown - dt)
    self.damage_flash = math.max(0, self.damage_flash - dt * 3)
    self.weapon_flash_timer = math.max(0, self.weapon_flash_timer - dt)
    self.wall_jump_timer = math.max(0, self.wall_jump_timer - dt)

    if self.weapon_flash_timer <= 0 and self.weapon_state == "firing" then
        self.weapon_state = "idle"
    end

    -- View punch spring
    self.view_punch_velocity = self.view_punch_velocity - self.view_punch * 80 * dt
    self.view_punch_velocity = self.view_punch_velocity * (1 - 8 * dt)
    self.view_punch = self.view_punch + self.view_punch_velocity * dt

    -- Keyboard turn (fallback)
    if love.keyboard.isDown("left") then self.angle = self.angle - TURN_SPEED * dt end
    if love.keyboard.isDown("right") then self.angle = self.angle + TURN_SPEED * dt end

    -- Sprint
    self.is_sprinting = love.keyboard.isDown("lshift", "rshift") and self.sprint_stamina > 0
    if self.is_sprinting then
        self.sprint_stamina = math.max(0, self.sprint_stamina - dt * 0.5)
        self.sprint_regen_delay = 0.8
    else
        self.sprint_regen_delay = math.max(0, self.sprint_regen_delay - dt)
        if self.sprint_regen_delay <= 0 then
            self.sprint_stamina = math.min(1, self.sprint_stamina + dt * 0.4)
        end
    end

    local speed = self.is_sprinting and SPRINT_SPEED or MOVE_SPEED

    -- Movement
    local move_x, move_y = 0, 0
    local cos_a = math.cos(self.angle)
    local sin_a = math.sin(self.angle)
    local moving = false

    if love.keyboard.isDown("w", "up") then
        move_x = move_x + cos_a; move_y = move_y + sin_a; moving = true
    end
    if love.keyboard.isDown("s", "down") then
        move_x = move_x - cos_a; move_y = move_y - sin_a; moving = true
    end
    if love.keyboard.isDown("a") then
        move_x = move_x + sin_a; move_y = move_y - cos_a; moving = true
    end
    if love.keyboard.isDown("d") then
        move_x = move_x - sin_a; move_y = move_y + cos_a; moving = true
    end

    if moving then self.move_time = self.move_time + dt end

    local len = math.sqrt(move_x * move_x + move_y * move_y)
    if len > 0 then
        move_x = move_x / len * speed * dt
        move_y = move_y / len * speed * dt
    end

    -- Add wall-jump push
    if math.abs(self.wall_push_vx) > 0.05 or math.abs(self.wall_push_vy) > 0.05 then
        move_x = move_x + self.wall_push_vx * dt
        move_y = move_y + self.wall_push_vy * dt
        self.wall_push_vx = self.wall_push_vx * (1 - 6 * dt)
        self.wall_push_vy = self.wall_push_vy * (1 - 6 * dt)
    end

    -- Recoil push
    if math.abs(self.recoil_vx) > 0.01 or math.abs(self.recoil_vy) > 0.01 then
        move_x = move_x + self.recoil_vx * dt
        move_y = move_y + self.recoil_vy * dt
        self.recoil_vx = self.recoil_vx * (1 - 10 * dt)
        self.recoil_vy = self.recoil_vy * (1 - 10 * dt)
    end

    -- Collision
    local new_x = self.x + move_x
    local new_y = self.y + move_y
    local r = COLLISION_RADIUS

    local blocked_x = map:is_solid(new_x + r, self.y) or map:is_solid(new_x - r, self.y)
    local blocked_y = map:is_solid(self.x, new_y + r) or map:is_solid(self.x, new_y - r)

    if not blocked_x then self.x = new_x end
    if not blocked_y then self.y = new_y end

    -- Pickup collision
    for _, sprite in ipairs(map.sprites) do
        if sprite.active and (sprite.type == "pickup_health" or sprite.type == "pickup_key" or sprite.type == "pickup_weapon") then
            local dx = sprite.x - self.x
            local dy = sprite.y - self.y
            if dx * dx + dy * dy < 0.5 then
                sprite.active = false
                if sprite.type == "pickup_health" then
                    self.health = math.min(self.max_health, self.health + 1)
                elseif sprite.type == "pickup_weapon" and sprite.weapon_name then
                    self:give_weapon(sprite.weapon_name)
                    self.current_weapon = #self.weapons
                end
            end
        end
    end

    -- Auto-fire
    local weapon = self:get_weapon()
    if weapon.auto and self.holding_fire and self.fire_cooldown <= 0 then
        self:fire(map)
    end
end

function FPSPlayer:try_wall_jump(map, sfx)
    if self.wall_jump_timer > 0 then return false end

    -- Check if we're against a wall
    local cos_a = math.cos(self.angle)
    local sin_a = math.sin(self.angle)
    local r = COLLISION_RADIUS + 0.15

    local wall_nx, wall_ny = 0, 0
    if map:is_solid(self.x + r, self.y) then wall_nx = -1
    elseif map:is_solid(self.x - r, self.y) then wall_nx = 1 end
    if map:is_solid(self.x, self.y + r) then wall_ny = -1
    elseif map:is_solid(self.x, self.y - r) then wall_ny = 1 end

    if wall_nx == 0 and wall_ny == 0 then return false end

    -- Push away from wall
    self.wall_push_vx = wall_nx * WALL_JUMP_PUSH
    self.wall_push_vy = wall_ny * WALL_JUMP_PUSH
    self.wall_jump_timer = WALL_JUMP_COOLDOWN
    self.view_punch_velocity = self.view_punch_velocity + 2

    if sfx then sfx:play("jump") end
    return true
end

function FPSPlayer:mousemoved(dx)
    if self.is_dead then return end
    self.angle = self.angle + dx * MOUSE_SENSITIVITY
end

function FPSPlayer:get_view_angle()
    return self.angle + self.view_punch * 0.03
end

function FPSPlayer:fire(map, sfx)
    if self.is_dead or self.fire_cooldown > 0 then return false end

    local weapon = self:get_weapon()
    self.fire_cooldown = weapon.fire_rate
    self.weapon_state = "firing"
    self.weapon_flash_timer = 0.08

    -- View punch and recoil
    self.view_punch_velocity = self.view_punch_velocity - weapon.recoil
    local cos_a = math.cos(self.angle)
    local sin_a = math.sin(self.angle)
    self.recoil_vx = -cos_a * weapon.recoil * 0.8
    self.recoil_vy = -sin_a * weapon.recoil * 0.8

    if sfx then sfx:play("shoot") end

    -- Hitscan
    local pellets = weapon.pellets or 1
    local any_hit = false

    for _ = 1, pellets do
        local spread_angle = self.angle + (math.random() - 0.5) * (weapon.spread or 0) * 2
        local shot_cos = math.cos(spread_angle)
        local shot_sin = math.sin(spread_angle)

        local best_dist = weapon.range
        local best_target = nil

        for _, sprite in ipairs(map.sprites) do
            if sprite.active and (sprite.type == "enemy" or sprite.type == "demon") then
                local dx = sprite.x - self.x
                local dy = sprite.y - self.y
                local dist = math.sqrt(dx * dx + dy * dy)

                if dist < best_dist then
                    local sa = math.atan2(dy, dx) - spread_angle
                    while sa > math.pi do sa = sa - math.pi * 2 end
                    while sa < -math.pi do sa = sa + math.pi * 2 end
                    local hit_a = math.atan2(0.4, dist)
                    if math.abs(sa) < hit_a then
                        best_dist = dist
                        best_target = sprite
                    end
                end
            end
        end

        if best_target then
            best_target.health = (best_target.health or 1) - weapon.damage
            if best_target.health <= 0 then
                best_target.active = false
                self.kills = self.kills + 1
                if sfx then sfx:play("stomp") end
            else
                if sfx then sfx:play("hit_enemy") end
            end
            any_hit = true
        end
    end

    return any_hit
end

function FPSPlayer:take_damage(amount, sfx)
    if self.is_dead then return end
    self.health = self.health - (amount or 1)
    self.damage_flash = 1
    self.view_punch_velocity = self.view_punch_velocity + (math.random() - 0.5) * 6
    if sfx then sfx:play("hit_enemy") end
    if self.health <= 0 then
        self.is_dead = true
        if sfx then sfx:play("death") end
    end
end

return FPSPlayer
