-- player.lua
-- Player movement, combat, and animation.

local Cooldown = require("cooldown")
local PlayerSpriteBank = require("player_sprite_bank")

local Player = {}
Player.__index = Player

local TILE_SIZE = 32
local WALK_SPEED = 250
local RUN_SPEED = 320
local GROUND_ACCEL = 3400
local GROUND_DECEL = 3800
local AIR_ACCEL = 2200
local AIR_DECEL = 1600
local GRAPPLE_SPEED = 1200
local GRAPPLE_MAX_LENGTH = 350
local GRAPPLE_PULL_SPEED = 860
local GRAPPLE_RELEASE_DISTANCE = 18
local COYOTE_TIME = 0.11
local JUMP_BUFFER_TIME = 0.12
local JUMP_CUT_MULTIPLIER = 0.45
local FALL_GRAVITY_MULTIPLIER = 1.22
local LOW_JUMP_GRAVITY_MULTIPLIER = 1.36
local RUN_DUST_INTERVAL = 0.08
local HURT_KNOCKBACK_X = 230
local HURT_KNOCKBACK_Y = -220
local function atan2(y, x)
    return math.atan(y, x)
end

local function approach(current, target, delta)
    if current < target then
        return math.min(current + delta, target)
    end
    if current > target then
        return math.max(current - delta, target)
    end
    return target
end

local function has_line_of_sight(level, x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    local distance = math.sqrt(dx * dx + dy * dy)
    local steps = math.max(1, math.floor(distance / 10))
    for step = 1, steps do
        local t = step / steps
        local sample_x = x1 + dx * t
        local sample_y = y1 + dy * t
        if level:get_tile(sample_x, sample_y) == 1 then
            return false
        end
    end
    return true
end

function Player:new()
    local class = self or Player
    local instance = setmetatable({}, { __index = class })
    instance.x, instance.y = 100, 100
    instance.width, instance.height = 28, 28
    instance.vx, instance.vy = 0, 0
    instance.jump_force = -550
    instance.gravity = 1600
    instance.on_ground = false
    instance.was_on_ground = false
    instance.on_wall = 0
    instance.wall_slide_speed = 100
    instance.wall_jump_force_x = 300
    instance.wall_jump_force_y = -500
    instance.facing = 1
    instance.fragments = 0
    instance.is_slowed = false

    instance.max_health = 3
    instance.health = instance.max_health
    instance.invincible_timer = 0
    instance.invincible_duration = 0.85
    instance.is_dead = false

    instance.shoot_cd = Cooldown:new(0.12)
    instance.grapple_cd = Cooldown:new(0.55)
    instance.grapple_state = "ready"
    instance.grapple_hook = { x = 0, y = 0, vx = 0, vy = 0 }
    instance.grapple_target = nil

    instance.coyote_timer = 0
    instance.jump_buffer_timer = 0
    instance.move_input = 0
    instance.run_dust_timer = 0
    instance.environment_force_x = 0
    instance.environment_force_y = 0
    instance.environment_label = nil
    instance.abilities = {
        shoot = true,
        grapple = true,
    }

    instance.sprite_bank = PlayerSpriteBank.load("assets/player/player_manifest.lua")
    instance.sprite_sheet = instance.sprite_bank.image
    instance.animations = instance.sprite_bank.animations
    instance.frame_size = instance.sprite_bank.frame_size
    instance.anim_timer = 0
    instance.anim_frame = 1
    instance.anim_state = "idle"
    instance.override_state = nil
    instance.override_hold = false
    instance:update_animation(0)

    return instance
end

function Player:set_start_pos(pos)
    self.x, self.y = pos.x, pos.y
end

function Player:apply_scene_rules(scene)
    local abilities = scene and scene.abilities or {}
    self.abilities = {
        shoot = abilities.shoot ~= false,
        grapple = abilities.grapple ~= false,
    }

    if not self.abilities.grapple then
        self:reset_grapple()
    end
end

function Player:get_base_animation_state()
    if not self.on_ground then
        if self.vy < -40 then
            return "jump"
        end
        return "fall"
    end

    if math.abs(self.vx) > 30 then
        return "run"
    end

    return "idle"
end

function Player:set_animation_state(state)
    if self.animations[state] == nil then
        return
    end

    if self.anim_state ~= state then
        self.anim_state = state
        self.anim_frame = 1
        self.anim_timer = 0
    end
end

function Player:resolve_animation_state()
    if self.override_state ~= nil then
        return self.override_state
    end
    return self:get_base_animation_state()
end

function Player:play_one_shot(state, hold_on_finish)
    if self.animations[state] == nil then
        return false
    end

    if self.override_state == "death" then
        return false
    end

    if state == "shoot" and (self.override_state == "hurt" or self.override_state == "death") then
        return false
    end

    self.override_state = state
    self.override_hold = hold_on_finish == true
    self:set_animation_state(state)
    return true
end

function Player:get_render_metrics()
    local animation = self.animations[self.anim_state]
    return animation and animation.render or self.sprite_bank.render
end

function Player:update_horizontal_velocity(dt)
    local current_speed = self.is_slowed and WALK_SPEED * 0.72 or RUN_SPEED

    self.move_input = 0
    if love.keyboard.isDown("a") then
        self.move_input = self.move_input - 1
    end
    if love.keyboard.isDown("d") then
        self.move_input = self.move_input + 1
    end

    if self.move_input ~= 0 then
        self.facing = self.move_input > 0 and 1 or -1
    end

    if self.grapple_state == "hooked" then
        return
    end

    local target = self.move_input * current_speed
    local accel = self.on_ground and GROUND_ACCEL or AIR_ACCEL
    local decel = self.on_ground and GROUND_DECEL or AIR_DECEL

    if self.move_input ~= 0 then
        self.vx = approach(self.vx, target, accel * dt)
    else
        self.vx = approach(self.vx, 0, decel * dt)
    end

    if self.environment_force_x ~= 0 then
        self.vx = self.vx + self.environment_force_x * dt
    end
end

function Player:update(dt, level)
    level:handle_tile_effects(self, dt)
    self.invincible_timer = math.max(0, self.invincible_timer - dt)
    self.jump_buffer_timer = math.max(0, self.jump_buffer_timer - dt)
    self.coyote_timer = math.max(0, self.coyote_timer - dt)

    self:update_horizontal_velocity(dt)

    if self.grapple_state == "firing" then
        self:update_grapple_firing(dt, level)
    elseif self.grapple_state == "hooked" then
        self:update_grapple_hooked(dt)
    else
        local gravity = self.gravity
        if self.vy > 0 then
            gravity = gravity * FALL_GRAVITY_MULTIPLIER
        elseif not love.keyboard.isDown("space") and self.vy < 0 then
            gravity = gravity * LOW_JUMP_GRAVITY_MULTIPLIER
        end
        self.vy = math.min(self.vy + gravity * dt, 1100)
    end

    if self.environment_force_y ~= 0 then
        self.vy = self.vy + self.environment_force_y * dt
    end

    local new_x = self.x + self.vx * dt
    local new_y = self.y + self.vy * dt

    self.was_on_ground = self.on_ground
    self.on_ground = false
    self.on_wall = 0

    if self.vy > 0 then
        if
            level:is_solid_at_pixel(self.x, new_y + self.height)
            or level:is_solid_at_pixel(self.x + self.width, new_y + self.height)
        then
            new_y = math.floor((new_y + self.height) / TILE_SIZE) * TILE_SIZE - self.height
            if self.vy > 240 and not self.was_on_ground then
                g_effects:spawn("land_dust", self.x + self.width / 2, new_y + self.height)
                g_sfx:play("land")
            end
            self.vy = 0
            self.on_ground = true
        end
    elseif self.vy < 0 then
        if
            level:is_solid_at_pixel(self.x, new_y)
            or level:is_solid_at_pixel(self.x + self.width, new_y)
        then
            new_y = (math.floor(new_y / TILE_SIZE) + 1) * TILE_SIZE
            self.vy = 0
        end
    end

    if self.vx > 0 then
        if
            level:is_solid_at_pixel(new_x + self.width, self.y)
            or level:is_solid_at_pixel(new_x + self.width, self.y + self.height - 1)
        then
            new_x = math.floor((new_x + self.width) / TILE_SIZE) * TILE_SIZE - self.width
            self.vx = 0
        end
    elseif self.vx < 0 then
        if
            level:is_solid_at_pixel(new_x, self.y)
            or level:is_solid_at_pixel(new_x, self.y + self.height - 1)
        then
            new_x = (math.floor(new_x / TILE_SIZE) + 1) * TILE_SIZE
            self.vx = 0
        end
    end

    self.x, self.y = new_x, new_y

    if not self.on_ground and self.vy > 0 then
        local wall_check_y = self.y + self.height / 2
        if level:is_solid_at_pixel(self.x - 1, wall_check_y) then
            self.on_wall = -1
        elseif level:is_solid_at_pixel(self.x + self.width + 1, wall_check_y) then
            self.on_wall = 1
        end

        local holding_toward_wall = (self.on_wall == -1 and self.move_input < 0)
            or (self.on_wall == 1 and self.move_input > 0)
        if self.on_wall ~= 0 and holding_toward_wall and self.vy > self.wall_slide_speed then
            self.vy = self.wall_slide_speed
            g_effects:spawn(
                "wall_slide_dust",
                self.x + (self.on_wall == 1 and self.width or 0),
                self.y + self.height / 2,
                { dir = -self.on_wall }
            )
        end
    end

    if self.on_ground then
        self.coyote_timer = COYOTE_TIME
    end

    if self.jump_buffer_timer > 0 then
        self:consume_jump()
    end

    self:update_run_dust(dt)
    self:update_animation(dt)
    self.shoot_cd:update(dt)
    self.grapple_cd:update(dt)
end

function Player:update_run_dust(dt)
    if self.on_ground and math.abs(self.vx) > 140 and self.move_input ~= 0 then
        self.run_dust_timer = self.run_dust_timer - dt
        if self.run_dust_timer <= 0 then
            g_effects:spawn("run_dust", self.x + self.width / 2, self.y + self.height)
            self.run_dust_timer = RUN_DUST_INTERVAL
        end
    else
        self.run_dust_timer = 0
    end
end

function Player:consume_jump()
    if self.grapple_state == "hooked" then
        self:reset_grapple()
        self.vy = self.jump_force * 0.8
    elseif self.on_wall ~= 0 and not self.on_ground then
        self.vx = self.wall_jump_force_x * -self.on_wall
        self.vy = self.wall_jump_force_y
        self.facing = -self.on_wall
    elseif self.on_ground or self.coyote_timer > 0 then
        self.vy = self.jump_force
        self.on_ground = false
    else
        return false
    end

    self.jump_buffer_timer = 0
    self.coyote_timer = 0
    g_effects:spawn("jump_dust", self.x + self.width / 2, self.y + self.height)
    g_sfx:play("jump")
    if g_run_stats then
        g_run_stats.jumps = (g_run_stats.jumps or 0) + 1
        g_run_stats.room_jumps = (g_run_stats.room_jumps or 0) + 1
    end
    return true
end

function Player:take_damage()
    if self.invincible_timer > 0 or self.is_dead then
        return false
    end

    self.health = self.health - 1
    self:reset_grapple()
    self.vx = -self.facing * HURT_KNOCKBACK_X
    self.vy = HURT_KNOCKBACK_Y
    g_sfx:play("hit_enemy")
    g_camera:shake(6, 0.16)
    g_effects:spawn("blood", self.x + self.width / 2, self.y + self.height / 2)

    if self.health <= 0 then
        self:die()
        return true
    end

    self.invincible_timer = self.invincible_duration
    self:play_one_shot("hurt", false)
    return false
end

function Player:die()
    self.is_dead = true
    self:reset_grapple()
    self:play_one_shot("death", true)
    g_sfx:play("death")
    g_effects:spawn("blood", self.x + self.width / 2, self.y + self.height / 2)
    if g_run_stats then
        g_run_stats.deaths = g_run_stats.deaths + 1
        g_run_stats.room_deaths = g_run_stats.room_deaths + 1
    end
end

function Player:respawn(pos)
    self.x, self.y = pos.x, pos.y
    self.vx, self.vy = 0, 0
    self.health = self.max_health
    self.is_dead = false
    self.invincible_timer = 0.6
    self.grapple_state = "ready"
    self.grapple_target = nil
    self.override_state = nil
    self.override_hold = false
    self.on_ground = false
    self.on_wall = 0
    self.coyote_timer = 0
    self.jump_buffer_timer = 0
    self.run_dust_timer = 0
    self:set_animation_state("idle")
end

function Player:is_invincible()
    return self.invincible_timer > 0
end

function Player:update_animation(dt)
    self:set_animation_state(self:resolve_animation_state())

    local animation = self.animations[self.anim_state]
    if animation == nil or #animation.frames == 0 then
        return
    end

    self.anim_timer = self.anim_timer + dt
    local frame_duration = 1 / math.max(animation.fps or 1, 1)

    while self.anim_timer >= frame_duration do
        self.anim_timer = self.anim_timer - frame_duration
        self.anim_frame = self.anim_frame + 1

        if self.anim_frame > #animation.frames then
            if animation.loop then
                self.anim_frame = 1
            else
                self.anim_frame = #animation.frames
                if self.override_state == self.anim_state and not self.override_hold then
                    self.override_state = nil
                    self:set_animation_state(self:get_base_animation_state())
                    animation = self.animations[self.anim_state]
                    if animation == nil or #animation.frames == 0 then
                        return
                    end
                    frame_duration = 1 / math.max(animation.fps or 1, 1)
                else
                    self.anim_timer = 0
                    break
                end
            end
        end
    end
end

function Player:update_grapple_firing(dt, level)
    self.grapple_hook.x = self.grapple_hook.x + self.grapple_hook.vx * dt
    self.grapple_hook.y = self.grapple_hook.y + self.grapple_hook.vy * dt

    local dx = self.grapple_hook.x - self.x
    local dy = self.grapple_hook.y - self.y
    local dist = math.sqrt(dx * dx + dy * dy)
    if dist > GRAPPLE_MAX_LENGTH then
        self:reset_grapple()
    else
        local anchor = level:get_grapple_anchor_at(self.grapple_hook.x, self.grapple_hook.y, 14)
        if anchor then
            self.grapple_state = "hooked"
            self.grapple_target = { x = anchor.x, y = anchor.y }
            if g_run_stats then
                g_run_stats.grapple_latches = (g_run_stats.grapple_latches or 0) + 1
                g_run_stats.room_grapple_latches = (g_run_stats.room_grapple_latches or 0) + 1
            end
        elseif level:is_solid_at_pixel(self.grapple_hook.x, self.grapple_hook.y) then
            self.grapple_state = "hooked"
            self.grapple_target = { x = self.grapple_hook.x, y = self.grapple_hook.y }
            if g_run_stats then
                g_run_stats.grapple_latches = (g_run_stats.grapple_latches or 0) + 1
                g_run_stats.room_grapple_latches = (g_run_stats.room_grapple_latches or 0) + 1
            end
        end
    end
end

function Player:update_grapple_hooked(dt)
    local player_center_x = self.x + self.width / 2
    local player_center_y = self.y + self.height / 2
    local dx = self.grapple_target.x - player_center_x
    local dy = self.grapple_target.y - player_center_y
    local dist = math.sqrt(dx * dx + dy * dy)

    if dist <= GRAPPLE_RELEASE_DISTANCE then
        self:reset_grapple()
        return
    end

    local nx = dx / math.max(dist, 0.001)
    local ny = dy / math.max(dist, 0.001)
    local desired_vx = nx * GRAPPLE_PULL_SPEED + self.move_input * 140
    local desired_vy = ny * GRAPPLE_PULL_SPEED

    self.vx = approach(self.vx, desired_vx, 4200 * dt)
    self.vy = approach(self.vy, desired_vy, 4600 * dt)

    if love.keyboard.isDown("space") and self.vy > -420 then
        self.vy = self.vy - 220 * dt
    end
end

function Player:keypressed(key)
    if key == "space" then
        self.jump_buffer_timer = JUMP_BUFFER_TIME
        if self.grapple_state == "hooked" then
            self:consume_jump()
        end
    end
end

function Player:keyreleased(key)
    if key == "space" and self.vy < -120 then
        self.vy = self.vy * JUMP_CUT_MULTIPLIER
    end
end

function Player:draw()
    if self.grapple_state == "firing" or self.grapple_state == "hooked" then
        love.graphics.setColor(0.72, 0.74, 0.78, 1)
        love.graphics.line(
            self.x + self.width / 2,
            self.y + self.height / 2,
            self.grapple_hook.x,
            self.grapple_hook.y
        )
        love.graphics.rectangle("fill", self.grapple_hook.x - 2, self.grapple_hook.y - 2, 4, 4)
    end

    if self.invincible_timer > 0 and math.floor(self.invincible_timer * 12) % 2 == 0 then
        love.graphics.setColor(1, 1, 1, 0.5)
    else
        love.graphics.setColor(1, 1, 1, 1)
    end

    local animation = self.animations[self.anim_state]
    local current_frame = animation and animation.frames[self.anim_frame]
    if current_frame then
        local render = self:get_render_metrics()
        local scale = render.scale or 1
        local offset = render.draw_offset or { x = 0, y = 0 }
        local draw_x = self.x + self.width / 2 + offset.x * scale * self.facing
        local draw_y = self.y + self.height + offset.y * scale
        love.graphics.draw(
            self.sprite_sheet,
            current_frame.quad,
            math.floor(draw_x),
            math.floor(draw_y),
            0,
            scale * self.facing,
            scale,
            self.frame_size / 2,
            render.feet_anchor or self.frame_size
        )
    else
        love.graphics.setColor(0.8, 0.2, 0.2, 1)
        love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function Player:fire_weapon(target_x, target_y)
    if not self.abilities.shoot or not self.shoot_cd:is_ready() or self.is_dead then
        return
    end

    self:reset_grapple()
    self.shoot_cd:use()
    self.facing = target_x < (self.x + self.width / 2) and -1 or 1

    local muzzle_x = self.x + self.width / 2 + self.facing * 8
    local muzzle_y = self.y + self.height / 2 - 2
    g_effects:spawn("muzzle_flash", muzzle_x, muzzle_y, { dir = self.facing })

    local snap_target_x = target_x
    local snap_target_y = target_y
    local best_distance = math.huge
    for _, enemy in ipairs(g_enemies.items) do
        local enemy_x = enemy.x + enemy.width / 2
        local enemy_y = enemy.y + enemy.height * 0.28
        local forward = (self.facing == 1 and enemy_x >= muzzle_x)
            or (self.facing == -1 and enemy_x <= muzzle_x)
        if forward then
            local dx = enemy_x - muzzle_x
            local dy = enemy_y - muzzle_y
            local distance = math.sqrt(dx * dx + dy * dy)
            if
                distance < 340
                and distance < best_distance
                and has_line_of_sight(g_level, muzzle_x, muzzle_y, enemy_x, enemy_y)
            then
                best_distance = distance
                snap_target_x = enemy_x
                snap_target_y = enemy_y
            end
        end
    end

    local angle = atan2(snap_target_y - muzzle_y, snap_target_x - muzzle_x)
    g_projectiles:spawn(muzzle_x, muzzle_y, snap_target_x, snap_target_y)
    self:play_one_shot("shoot", false)
    if g_run_stats then
        g_run_stats.shots_fired = g_run_stats.shots_fired + 1
        g_run_stats.room_shots_fired = g_run_stats.room_shots_fired + 1
    end

    local recoil = self.on_ground and 24 or 40
    self.vx = self.vx - math.cos(angle) * recoil
    g_camera:shake(3, 0.08)
    g_ui:trigger_flash()
end

function Player:fire_grapple(target_x, target_y)
    if not self.abilities.grapple or not self.grapple_cd:is_ready() or self.is_dead then
        return
    end

    self.grapple_cd:use()
    self.grapple_state = "firing"
    self.grapple_hook.x = self.x + self.width / 2
    self.grapple_hook.y = self.y + self.height / 2
    local angle = atan2(target_y - self.grapple_hook.y, target_x - self.grapple_hook.x)
    self.grapple_hook.vx = math.cos(angle) * GRAPPLE_SPEED
    self.grapple_hook.vy = math.sin(angle) * GRAPPLE_SPEED
    g_sfx:play("grapple")
    if g_run_stats then
        g_run_stats.grapples_fired = (g_run_stats.grapples_fired or 0) + 1
        g_run_stats.room_grapples_fired = (g_run_stats.room_grapples_fired or 0) + 1
    end
end

function Player:reset_grapple()
    self.grapple_state = "ready"
    self.grapple_target = nil
end

function Player:collect_fragment()
    self.fragments = self.fragments + 1
end

return Player
