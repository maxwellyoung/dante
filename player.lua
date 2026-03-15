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
local GRAPPLE_MAX_LENGTH = 460
local GRAPPLE_PULL_SPEED = 980
local GRAPPLE_RELEASE_DISTANCE = 18
local GRAPPLE_AIM_SNAP_RADIUS = 64
local GRAPPLE_LATCH_RADIUS = 24
local GRAPPLE_AUTO_ANGLE_DOT = 0.62
local GRAPPLE_AUTO_BIAS = 180
local COYOTE_TIME = 0.11
local JUMP_BUFFER_TIME = 0.12
local JUMP_CUT_MULTIPLIER = 0.45
local FALL_GRAVITY_MULTIPLIER = 1.22
local LOW_JUMP_GRAVITY_MULTIPLIER = 1.36
local WALL_GRACE_TIME = 0.12
local WALL_JUMP_LOCK_TIME = 0.14
local WALL_DETACH_TIME = 0.1
local WALL_STICK_TIME = 0.14
local WALL_AUTO_DETACH_TIME = 0.18
local WALL_AUTO_DETACH_PUSH = 90
local WALL_CONTACT_INSET = 5
local RUN_DUST_INTERVAL = 0.08
local HURT_KNOCKBACK_X = 230
local HURT_KNOCKBACK_Y = -220
local SHOT_RECOIL_GROUND = 16
local SHOT_RECOIL_AIR = 26
local WALL_JUMP_NUDGE_X = 8
local MAX_UPDATE_STEP = 1 / 120
local DASH_SPEED = 760
local DASH_DURATION = 0.11
local DASH_COOLDOWN = 0.08
local DASH_IMPACT_WINDOW = 0.08
local DASH_WALL_HITSTOP = 0.05
local DASH_FLOOR_HITSTOP = 0.03
local CROUCH_SPEED = 92
local ROLL_SPEED = 430
local ROLL_DURATION = 0.28
local ROLL_COOLDOWN = 0.24
local math_atan2 = rawget(math, "atan2")
local function atan2(y, x)
    if math_atan2 then
        return math_atan2(y, x)
    end
    if x > 0 then
        return math.atan(y / x)
    end
    if x < 0 then
        return math.atan(y / x) + (y >= 0 and math.pi or -math.pi)
    end
    if y > 0 then
        return math.pi / 2
    end
    if y < 0 then
        return -math.pi / 2
    end
    return 0
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

local function get_level(self)
    return self.services and self.services.level or g_level
end

local function get_effects(self)
    return self.services and self.services.effects or g_effects
end

local function get_sfx(self)
    return self.services and self.services.sfx or g_sfx
end

local function get_camera(self)
    return self.services and self.services.camera or g_camera
end

local function get_ui(self)
    return self.services and self.services.ui or g_ui
end

local function get_enemies(self)
    return self.services and self.services.enemies or g_enemies
end

local function get_projectiles(self)
    return self.services and self.services.projectiles or g_projectiles
end

local function get_run_stats(self)
    return self.services and self.services.run_stats or g_run_stats
end

local function get_trigger_hitstop(self)
    return self.services and self.services.trigger_hitstop or g_trigger_hitstop
end

local function mark_meaningful_action(self)
    local marker = self.services and self.services.mark_meaningful_action or g_mark_meaningful_action
    if marker then
        marker()
    end
end

local function record_removed_ability_notice(self)
    local record = self.services and self.services.record_removed_ability_notice or g_record_removed_ability_notice
    if record then
        record()
    end
end

function Player:new(services)
    local class = self or Player
    local instance = setmetatable({}, { __index = class })
    instance.services = services
    instance.x, instance.y = 100, 100
    instance.width, instance.height = 28, 28
    instance.vx, instance.vy = 0, 0
    instance.jump_force = -550
    instance.gravity = 1600
    instance.on_ground = false
    instance.was_on_ground = false
    instance.on_wall = 0
    instance.last_wall = 0
    instance.wall_grace_timer = 0
    instance.wall_jump_lock_timer = 0
    instance.wall_jump_lock_dir = 0
    instance.wall_detach_timer = 0
    instance.wall_slide_hold_timer = 0
    instance.wall_slide_speed = 180
    instance.wall_jump_force_x = 360
    instance.wall_jump_force_y = -520
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
    instance.grapple_aim_anchor = nil
    instance.dash_available = true
    instance.dash_timer = 0
    instance.dash_dir_x = 0
    instance.dash_dir_y = 0
    instance.dash_cooldown = 0
    instance.dash_impact_timer = 0
    instance.crouch_input = false
    instance.is_crouching = false
    instance.roll_timer = 0
    instance.roll_cooldown = 0
    instance.roll_dir = 1

    instance.coyote_timer = 0
    instance.jump_buffer_timer = 0
    instance.move_input = 0
    instance.run_dust_timer = 0
    instance.environment_force_x = 0
    instance.environment_force_y = 0
    instance.environment_label = nil
    instance.seen_environment_labels = {}
    instance.removed_ability_notice_shown = false
    instance.abilities = {
        shoot = true,
        grapple = true,
        dash = true,
        crouch = true,
        roll = true,
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
        dash = abilities.dash ~= false,
        crouch = abilities.crouch ~= false,
        roll = abilities.roll ~= false,
    }
    self.removed_ability_notice_shown = false
    self.seen_environment_labels = {}

    if not self.abilities.grapple then
        self:reset_grapple()
    end
    if not self.abilities.dash then
        self.dash_available = false
        self.dash_timer = 0
        self.dash_dir_x = 0
        self.dash_dir_y = 0
    end
    self.is_crouching = false
    self.crouch_input = false
    self.roll_timer = 0
    self.roll_cooldown = 0
end

function Player:get_base_animation_state()
    if self.roll_timer > 0 then
        return "run"
    end
    if self.is_crouching then
        return "idle"
    end
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
    self.crouch_input = love.keyboard.isDown("s", "down")
    self.is_crouching = self.abilities.crouch ~= false
        and self.crouch_input
        and self.on_ground
        and self.dash_timer <= 0
        and self.roll_timer <= 0
        and self.grapple_state ~= "hooked"
        and not self.is_dead

    if self.dash_timer > 0 then
        self.vx = self.dash_dir_x * DASH_SPEED
        self.vy = self.dash_dir_y * DASH_SPEED
        return
    end
    if self.roll_timer > 0 then
        self.vx = self.roll_dir * ROLL_SPEED
        return
    end

    local current_speed = self.is_slowed and WALK_SPEED * 0.72 or RUN_SPEED
    if self.is_crouching then
        current_speed = CROUCH_SPEED
    end

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
    if self.wall_jump_lock_timer > 0 and self.move_input == self.wall_jump_lock_dir then
        target = 0
    end
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

function Player:is_holding_toward_wall(dir)
    return (dir == -1 and self.move_input < 0) or (dir == 1 and self.move_input > 0)
end

function Player:get_wall_contact(level)
    if self.on_ground or self.wall_detach_timer > 0 then
        return 0
    end

    local sample_top = self.y + WALL_CONTACT_INSET
    local sample_mid = self.y + self.height / 2
    local sample_bottom = self.y + self.height - WALL_CONTACT_INSET
    local left_x = self.x - 2
    local right_x = self.x + self.width + 2

    local function side_is_solid(sample_x)
        return level:is_solid_at_pixel(sample_x, sample_top)
            or level:is_solid_at_pixel(sample_x, sample_mid)
            or level:is_solid_at_pixel(sample_x, sample_bottom)
    end

    local left_locked = self.wall_jump_lock_timer > 0 and self.wall_jump_lock_dir == -1
    local right_locked = self.wall_jump_lock_timer > 0 and self.wall_jump_lock_dir == 1

    if not left_locked and side_is_solid(left_x) then
        return -1
    end
    if not right_locked and side_is_solid(right_x) then
        return 1
    end

    return 0
end

function Player:update_wall_state(dt, level)
    if self.on_ground then
        self.on_wall = 0
        self.last_wall = 0
        self.wall_grace_timer = 0
        self.wall_slide_hold_timer = 0
        return
    end

    local wall_contact = self:get_wall_contact(level)
    if wall_contact ~= 0 then
        self.on_wall = wall_contact
        self.last_wall = wall_contact
        self.wall_grace_timer = WALL_GRACE_TIME
    else
        self.on_wall = 0
    end

    local holding_toward_wall = self:is_holding_toward_wall(self.on_wall)
    local can_slide = self.on_wall ~= 0 and holding_toward_wall and self.vy > 90 and self.wall_detach_timer <= 0
    if can_slide then
        self.wall_slide_hold_timer = self.wall_slide_hold_timer + dt
        if self.wall_slide_hold_timer >= WALL_STICK_TIME then
            self.wall_detach_timer = math.max(self.wall_detach_timer, WALL_AUTO_DETACH_TIME)
            self.vx = -self.on_wall * math.max(WALL_AUTO_DETACH_PUSH, math.abs(self.vx) * 0.25)
            self.on_wall = 0
            self.wall_slide_hold_timer = 0
            return
        end
        self.vy = approach(self.vy, self.wall_slide_speed, 2800 * dt)
        if self.vy > self.wall_slide_speed + 10 then
            local effects = get_effects(self)
            if effects then
                effects:spawn(
                    "wall_slide_dust",
                    self.x + (self.on_wall == 1 and self.width or 0),
                    self.y + self.height / 2,
                    { dir = -self.on_wall }
                )
            end
        end
    else
        self.wall_slide_hold_timer = 0
    end
end

function Player:get_wall_jump_dir()
    if self.on_wall ~= 0 then
        return self.on_wall
    end
    if self.wall_grace_timer > 0 then
        return self.last_wall
    end
    return 0
end

function Player:update_step(dt, level)
    level:handle_tile_effects(self, dt)
    self.invincible_timer = math.max(0, self.invincible_timer - dt)
    self.jump_buffer_timer = math.max(0, self.jump_buffer_timer - dt)
    self.coyote_timer = math.max(0, self.coyote_timer - dt)
    self.wall_grace_timer = math.max(0, self.wall_grace_timer - dt)
    self.wall_jump_lock_timer = math.max(0, self.wall_jump_lock_timer - dt)
    self.wall_detach_timer = math.max(0, self.wall_detach_timer - dt)
    self.dash_timer = math.max(0, self.dash_timer - dt)
    self.dash_cooldown = math.max(0, self.dash_cooldown - dt)
    self.dash_impact_timer = math.max(0, self.dash_impact_timer - dt)
    self.roll_timer = math.max(0, self.roll_timer - dt)
    self.roll_cooldown = math.max(0, self.roll_cooldown - dt)
    if self.wall_jump_lock_timer == 0 then
        self.wall_jump_lock_dir = 0
    end

    self:update_horizontal_velocity(dt)

    if self.dash_timer > 0 then
        self.vx = self.dash_dir_x * DASH_SPEED
        self.vy = self.dash_dir_y * DASH_SPEED
    elseif self.grapple_state == "firing" then
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

    if self.environment_force_y ~= 0 and self.dash_timer == 0 then
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
                local effects = get_effects(self)
                local sfx = get_sfx(self)
                if effects then
                    effects:spawn("land_dust", self.x + self.width / 2, new_y + self.height)
                end
                if sfx then
                    sfx:play("land")
                end
            end
            if self.dash_timer > 0 then
                self:dash_impact("floor", self.x + self.width / 2, new_y + self.height)
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
            if self.dash_timer > 0 then
                self:dash_impact("wall", new_x + self.width, self.y + self.height / 2)
            end
            new_x = math.floor((new_x + self.width) / TILE_SIZE) * TILE_SIZE - self.width
            self.vx = 0
        end
    elseif self.vx < 0 then
        if
            level:is_solid_at_pixel(new_x, self.y)
            or level:is_solid_at_pixel(new_x, self.y + self.height - 1)
        then
            if self.dash_timer > 0 then
                self:dash_impact("wall", new_x, self.y + self.height / 2)
            end
            new_x = (math.floor(new_x / TILE_SIZE) + 1) * TILE_SIZE
            self.vx = 0
        end
    end

    if self.wall_detach_timer > 0 and self.wall_jump_lock_dir ~= 0 then
        local detach_push = -self.wall_jump_lock_dir * 70
        if (detach_push < 0 and self.vx > detach_push) or (detach_push > 0 and self.vx < detach_push) then
            self.vx = detach_push
        end
    end

    self.x, self.y = new_x, new_y

    self:update_wall_state(dt, level)

    if not self.on_ground then
        self.is_crouching = false
    end

    if self.on_ground or self.on_wall ~= 0 or self.grapple_state == "hooked" then
        self.dash_available = self.abilities.dash ~= false
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

function Player:update(dt, level)
    local remaining = dt
    while remaining > 0 do
        local step = math.min(remaining, MAX_UPDATE_STEP)
        self:update_step(step, level)
        remaining = remaining - step
    end
end

function Player:update_run_dust(dt)
    if self.on_ground and math.abs(self.vx) > 140 and self.move_input ~= 0 then
        self.run_dust_timer = self.run_dust_timer - dt
        if self.run_dust_timer <= 0 then
            local effects = get_effects(self)
            if effects then
                effects:spawn("run_dust", self.x + self.width / 2, self.y + self.height)
            end
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
    elseif not self.on_ground and self.wall_grace_timer > 0 and self:get_wall_jump_dir() ~= 0 then
        local wall_dir = self:get_wall_jump_dir()
        self.vx = self.wall_jump_force_x * -wall_dir
        self.vy = self.wall_jump_force_y
        self.facing = -wall_dir
        self.x = self.x - wall_dir * WALL_JUMP_NUDGE_X
        self.on_wall = 0
        self.last_wall = 0
        self.wall_grace_timer = 0
        self.wall_jump_lock_timer = WALL_JUMP_LOCK_TIME
        self.wall_jump_lock_dir = wall_dir
        self.wall_detach_timer = WALL_DETACH_TIME
    elseif self.on_ground or self.coyote_timer > 0 then
        self.vy = self.jump_force
        self.on_ground = false
    else
        return false
    end

    self.jump_buffer_timer = 0
    self.coyote_timer = 0
    local effects = get_effects(self)
    local sfx = get_sfx(self)
    local run_stats = get_run_stats(self)
    if effects then
        effects:spawn("jump_dust", self.x + self.width / 2, self.y + self.height)
    end
    if sfx then
        sfx:play("jump")
    end
    if run_stats then
        run_stats.jumps = (run_stats.jumps or 0) + 1
        run_stats.room_jumps = (run_stats.room_jumps or 0) + 1
    end
    mark_meaningful_action(self)
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
    local sfx = get_sfx(self)
    local camera = get_camera(self)
    local effects = get_effects(self)
    if sfx then
        sfx:play("hit_enemy")
    end
    if camera then
        camera:shake(6, 0.16)
    end
    if effects then
        effects:spawn("blood", self.x + self.width / 2, self.y + self.height / 2)
    end

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
    local sfx = get_sfx(self)
    local effects = get_effects(self)
    local run_stats = get_run_stats(self)
    if sfx then
        sfx:play("death")
    end
    if effects then
        effects:spawn("blood", self.x + self.width / 2, self.y + self.height / 2)
    end
    if run_stats then
        run_stats.deaths = run_stats.deaths + 1
        run_stats.room_deaths = run_stats.room_deaths + 1
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
    self.last_wall = 0
    self.environment_label = nil
    self.seen_environment_labels = {}
    self.coyote_timer = 0
    self.jump_buffer_timer = 0
    self.wall_grace_timer = 0
    self.wall_jump_lock_timer = 0
    self.wall_jump_lock_dir = 0
    self.wall_detach_timer = 0
    self.wall_slide_hold_timer = 0
    self.run_dust_timer = 0
    self.dash_available = self.abilities.dash ~= false
    self.dash_timer = 0
    self.dash_dir_x = 0
    self.dash_dir_y = 0
    self.dash_cooldown = 0
    self.dash_impact_timer = 0
    self.crouch_input = false
    self.is_crouching = false
    self.roll_timer = 0
    self.roll_cooldown = 0
    self.roll_dir = self.facing
    self.grapple_hook.x = self.x + self.width / 2
    self.grapple_hook.y = self.y + self.height / 2
    self.grapple_hook.vx = 0
    self.grapple_hook.vy = 0
    self.grapple_aim_anchor = nil
    self.shoot_cd.timer = 0
    self.grapple_cd.timer = 0
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
    local latch_radius = level.room_type == "traversal" and 44 or GRAPPLE_LATCH_RADIUS

    local dx = self.grapple_hook.x - self.x
    local dy = self.grapple_hook.y - self.y
    local dist = math.sqrt(dx * dx + dy * dy)
    if dist > GRAPPLE_MAX_LENGTH then
        self:reset_grapple()
    else
        local anchor = self.grapple_aim_anchor
            or level:get_grapple_anchor_at(
                self.grapple_hook.x,
                self.grapple_hook.y,
                latch_radius
            )
        if anchor then
            local anchor_dx = anchor.x - self.grapple_hook.x
            local anchor_dy = anchor.y - self.grapple_hook.y
            if anchor_dx * anchor_dx + anchor_dy * anchor_dy <= latch_radius * latch_radius then
                self.grapple_state = "hooked"
                self.grapple_target = { x = anchor.x, y = anchor.y }
                self.grapple_hook.x = anchor.x
                self.grapple_hook.y = anchor.y
                self.grapple_aim_anchor = nil
                local effects = get_effects(self)
                local camera = get_camera(self)
                local run_stats = get_run_stats(self)
                if effects then
                    effects:spawn("sparks", anchor.x, anchor.y)
                end
                if camera then
                    camera:shake(2, 0.06)
                end
                if run_stats then
                    run_stats.grapple_latches = (run_stats.grapple_latches or 0) + 1
                    run_stats.room_grapple_latches = (run_stats.room_grapple_latches or 0) + 1
                end
                mark_meaningful_action(self)
            end
        elseif level:is_solid_at_pixel(self.grapple_hook.x, self.grapple_hook.y) then
            self:reset_grapple()
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
    local desired_vx = nx * GRAPPLE_PULL_SPEED + self.move_input * 220
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
        if
            self.grapple_state == "hooked"
            or self.on_ground
            or self.coyote_timer > 0
            or self:get_wall_jump_dir() ~= 0
        then
            self:consume_jump()
        end
    elseif key == "e" then
        self:fire_grapple_auto()
    elseif key == "lshift" then
        self:dash()
    elseif key == "lctrl" or key == "rctrl" or key == "c" then
        self:roll()
    end
end

function Player:keyreleased(key)
    if key == "space" and self.vy < -120 then
        self.vy = self.vy * JUMP_CUT_MULTIPLIER
    end
end

function Player:draw()
    if self.grapple_state == "firing" or self.grapple_state == "hooked" then
        local hook_x = self.grapple_state == "hooked" and self.grapple_target.x or self.grapple_hook.x
        local hook_y = self.grapple_state == "hooked" and self.grapple_target.y or self.grapple_hook.y
        love.graphics.setColor(0.72, 0.74, 0.78, 1)
        love.graphics.line(
            self.x + self.width / 2,
            self.y + self.height / 2,
            hook_x,
            hook_y
        )
        love.graphics.rectangle("fill", hook_x - 2, hook_y - 2, 4, 4)
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
        local scale_x = scale * self.facing
        local scale_y = scale
        if self.roll_timer > 0 then
            scale_x = scale_x * 1.08
            scale_y = scale_y * 0.72
        elseif self.is_crouching then
            scale_y = scale_y * 0.8
        end
        if self.roll_timer > 0 then
            love.graphics.setColor(0.95, 0.56, 0.22, 0.22)
            love.graphics.ellipse(
                "fill",
                math.floor(self.x + self.width / 2),
                math.floor(self.y + self.height - 2),
                18,
                6
            )
            love.graphics.setColor(1, 1, 1, 1)
        elseif self.is_crouching then
            love.graphics.setColor(0.55, 0.74, 0.96, 0.18)
            love.graphics.rectangle(
                "fill",
                math.floor(self.x - 1),
                math.floor(self.y + self.height - 7),
                self.width + 2,
                6,
                2,
                2
            )
            love.graphics.setColor(1, 1, 1, 1)
        elseif self.on_wall ~= 0 and not self.on_ground then
            love.graphics.setColor(0.96, 0.9, 0.64, 0.16)
            love.graphics.rectangle(
                "fill",
                math.floor(self.x + (self.on_wall == 1 and self.width or -3)),
                math.floor(self.y + 6),
                3,
                self.height - 12,
                2,
                2
            )
            love.graphics.setColor(1, 1, 1, 1)
        end
        love.graphics.draw(
            self.sprite_sheet,
            current_frame.quad,
            math.floor(draw_x),
            math.floor(draw_y),
            0,
            scale_x,
            scale_y,
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
    if not self.abilities.shoot or not self.shoot_cd:is_ready() or self.is_dead or self.roll_timer > 0 then
        return
    end

    self:reset_grapple()
    self.shoot_cd:use()
    local shot_dir = target_x < (self.x + self.width / 2) and -1 or 1
    if self.move_input == 0 then
        self.facing = shot_dir
    end

    local muzzle_x = self.x + self.width / 2 + shot_dir * 8
    local muzzle_y = self.y + self.height / 2 - 2
    local effects = get_effects(self)
    local enemies = get_enemies(self)
    local level = get_level(self)
    local projectiles = get_projectiles(self)
    local run_stats = get_run_stats(self)
    local camera = get_camera(self)
    local ui = get_ui(self)
    if effects then
        effects:spawn("muzzle_flash", muzzle_x, muzzle_y, { dir = shot_dir })
    end

    local snap_target_x = target_x
    local snap_target_y = target_y
    local best_distance = math.huge
    local weapon_profile = level and level.weapon_profile or nil
    local auto_aim_enabled = weapon_profile == nil or weapon_profile.auto_aim ~= false
    if auto_aim_enabled then
        for _, enemy in ipairs(enemies and enemies.items or {}) do
            local enemy_x = enemy.x + enemy.width / 2
            local enemy_y = enemy.y + enemy.height * 0.28
            local forward = (shot_dir == 1 and enemy_x >= muzzle_x)
                or (shot_dir == -1 and enemy_x <= muzzle_x)
            if forward then
                local dx = enemy_x - muzzle_x
                local dy = enemy_y - muzzle_y
                local distance = math.sqrt(dx * dx + dy * dy)
                if
                    distance < 340
                    and distance < best_distance
                    and level
                    and has_line_of_sight(level, muzzle_x, muzzle_y, enemy_x, enemy_y)
                then
                    best_distance = distance
                    snap_target_x = enemy_x
                    snap_target_y = enemy_y
                end
            end
        end
    end

    local angle = atan2(snap_target_y - muzzle_y, snap_target_x - muzzle_x)
    if projectiles then
        projectiles:spawn(muzzle_x, muzzle_y, snap_target_x, snap_target_y, weapon_profile)
    end
    self:play_one_shot("shoot", false)
    if run_stats then
        run_stats.shots_fired = run_stats.shots_fired + 1
        run_stats.room_shots_fired = run_stats.room_shots_fired + 1
    end
    mark_meaningful_action(self)

    local recoil = self.on_ground and SHOT_RECOIL_GROUND or SHOT_RECOIL_AIR
    self.vx = self.vx - math.cos(angle) * recoil
    if camera then
        camera:shake(3, 0.08)
    end
    if ui then
        ui:trigger_flash()
    end
end

function Player:get_dash_vector()
    local dir_x = 0
    local dir_y = 0
    if love.keyboard.isDown("a") then
        dir_x = dir_x - 1
    end
    if love.keyboard.isDown("d") then
        dir_x = dir_x + 1
    end
    if love.keyboard.isDown("w") then
        dir_y = dir_y - 1
    end
    if love.keyboard.isDown("s") then
        dir_y = dir_y + 1
    end
    if dir_x == 0 and dir_y == 0 then
        dir_x = self.facing ~= 0 and self.facing or 1
    end
    local length = math.sqrt(dir_x * dir_x + dir_y * dir_y)
    return dir_x / math.max(length, 0.001), dir_y / math.max(length, 0.001)
end

function Player:dash_impact(kind, x, y)
    if self.dash_impact_timer > 0 then
        return
    end
    self.dash_timer = 0
    self.dash_cooldown = DASH_COOLDOWN
    self.dash_impact_timer = DASH_IMPACT_WINDOW
    if kind == "wall" then
        self.vx = -self.dash_dir_x * 120
        self.vy = math.min(self.vy * 0.12, 60)
    else
        self.vx = self.vx * 0.32
        self.vy = -110
    end
    local effects = get_effects(self)
    local sfx = get_sfx(self)
    local camera = get_camera(self)
    if effects then
        effects:spawn("dash_impact", x, y, { kind = kind })
        effects:spawn("sparks", x, y)
    end
    if sfx then
        sfx:play("hit_wall")
    end
    if camera then
        camera:shake(kind == "wall" and 6 or 4, 0.08)
    end
    local trigger_hitstop = get_trigger_hitstop(self)
    if trigger_hitstop then
        trigger_hitstop(kind == "wall" and DASH_WALL_HITSTOP or DASH_FLOOR_HITSTOP)
    end
end

function Player:dash()
    if
        not self.abilities.dash
        or self.is_dead
        or not self.dash_available
        or self.dash_cooldown > 0
        or self.roll_timer > 0
    then
        return false
    end

    local dir_x, dir_y = self:get_dash_vector()
    self:reset_grapple()
    self.is_crouching = false
    self.dash_available = false
    self.dash_timer = DASH_DURATION
    self.dash_cooldown = DASH_COOLDOWN
    self.dash_dir_x = dir_x
    self.dash_dir_y = dir_y
    self.vx = dir_x * DASH_SPEED
    self.vy = dir_y * DASH_SPEED
    self.on_ground = false
    self.coyote_timer = 0
    self.wall_grace_timer = 0
    self.wall_jump_lock_timer = 0
    self.wall_jump_lock_dir = 0
    self.wall_detach_timer = 0
    if math.abs(dir_x) > 0.1 then
        self.facing = dir_x > 0 and 1 or -1
    end
    local effects = get_effects(self)
    local sfx = get_sfx(self)
    local camera = get_camera(self)
    local ui = get_ui(self)
    local run_stats = get_run_stats(self)
    if effects then
        effects:spawn("dash_burst", self.x + self.width / 2, self.y + self.height / 2, {
            dir_x = dir_x,
            dir_y = dir_y,
        })
        effects:spawn("jump_dust", self.x + self.width / 2, self.y + self.height / 2)
    end
    if sfx then
        sfx:play("dash")
    end
    if camera then
        camera:shake(4, 0.07)
    end
    if ui then
        ui:trigger_flash()
    end
    if run_stats then
        run_stats.dashes = (run_stats.dashes or 0) + 1
        run_stats.room_dashes = (run_stats.room_dashes or 0) + 1
    end
    mark_meaningful_action(self)
    return true
end

function Player:roll()
    if
        self.is_dead
        or not self.on_ground
        or self.abilities.roll == false
        or self.roll_timer > 0
        or self.roll_cooldown > 0
        or self.dash_timer > 0
        or self.grapple_state == "hooked"
    then
        return false
    end

    self:reset_grapple()
    self.is_crouching = false
    self.roll_timer = ROLL_DURATION
    self.roll_cooldown = ROLL_COOLDOWN
    self.roll_dir = self.move_input ~= 0 and self.move_input or (self.facing ~= 0 and self.facing or 1)
    self.facing = self.roll_dir
    self.vx = self.roll_dir * ROLL_SPEED

    local effects = get_effects(self)
    local sfx = get_sfx(self)
    if effects then
        effects:spawn("run_dust", self.x + self.width / 2, self.y + self.height)
    end
    if sfx then
        sfx:play("dash")
    end
    local run_stats = get_run_stats(self)
    if run_stats then
        run_stats.rolls = (run_stats.rolls or 0) + 1
        run_stats.room_rolls = (run_stats.room_rolls or 0) + 1
    end
    mark_meaningful_action(self)
    return true
end

function Player:fire_grapple(target_x, target_y)
    if self.roll_timer > 0 then
        return
    end
    if not self.abilities.grapple then
        local level = get_level(self)
        local ui = get_ui(self)
        if not self.removed_ability_notice_shown and level and level.removed_ability ~= "none" then
            self.removed_ability_notice_shown = true
            if ui then
                ui:show_story_callout(
                    level.scene_title,
                    "The " .. level.removed_ability .. " is gone. " .. level.environment_hook .. " decides the route now.",
                    level.accent_color,
                    2.4
                )
            end
            record_removed_ability_notice(self)
        end
        return
    end
    if not self.grapple_cd:is_ready() or self.is_dead then
        return
    end

    local anchor = self:find_best_grapple_anchor(target_x, target_y)
    if not anchor then
        anchor = self:find_fallback_grapple_anchor()
    end
    if anchor then
        target_x = anchor.x
        target_y = anchor.y
    end

    self.grapple_cd:use()
    self.grapple_state = "firing"
    self.grapple_hook.x = self.x + self.width / 2
    self.grapple_hook.y = self.y + self.height / 2
    self.grapple_aim_anchor = anchor and { x = anchor.x, y = anchor.y } or nil
    local angle = atan2(target_y - self.grapple_hook.y, target_x - self.grapple_hook.x)
    self.grapple_hook.vx = math.cos(angle) * GRAPPLE_SPEED
    self.grapple_hook.vy = math.sin(angle) * GRAPPLE_SPEED
    local sfx = get_sfx(self)
    local run_stats = get_run_stats(self)
    if sfx then
        sfx:play("grapple")
    end
    if run_stats then
        run_stats.grapples_fired = (run_stats.grapples_fired or 0) + 1
        run_stats.room_grapples_fired = (run_stats.room_grapples_fired or 0) + 1
    end
    mark_meaningful_action(self)
end

function Player:find_best_grapple_anchor(target_x, target_y)
    local level = get_level(self)
    if not level or not level.grapple_anchors or #level.grapple_anchors == 0 then
        return nil
    end

    local origin_x = self.x + self.width / 2
    local origin_y = self.y + self.height / 2
    local direct = level:get_grapple_anchor_at(target_x, target_y, GRAPPLE_AIM_SNAP_RADIUS)
    if direct then
        return direct
    end

    local aim_dx = target_x - origin_x
    local aim_dy = target_y - origin_y
    local aim_length = math.sqrt(aim_dx * aim_dx + aim_dy * aim_dy)
    if aim_length < 0.001 then
        aim_dx = self.facing
        aim_dy = -0.35
        aim_length = math.sqrt(aim_dx * aim_dx + aim_dy * aim_dy)
    end
    aim_dx = aim_dx / aim_length
    aim_dy = aim_dy / aim_length

    local best_anchor = nil
    local best_score = math.huge
    for _, anchor in ipairs(level.grapple_anchors) do
        local dx = anchor.x - origin_x
        local dy = anchor.y - origin_y
        local distance = math.sqrt(dx * dx + dy * dy)
        if distance <= GRAPPLE_MAX_LENGTH and has_line_of_sight(level, origin_x, origin_y, anchor.x, anchor.y) then
            local dot = ((dx / math.max(distance, 0.001)) * aim_dx) + ((dy / math.max(distance, 0.001)) * aim_dy)
            if dot >= GRAPPLE_AUTO_ANGLE_DOT then
                local score = distance - (dot * GRAPPLE_AUTO_BIAS)
                if score < best_score then
                    best_score = score
                    best_anchor = anchor
                end
            end
        end
    end

    return best_anchor
end

function Player:find_fallback_grapple_anchor()
    local level = get_level(self)
    if not level or not level.grapple_anchors or #level.grapple_anchors == 0 then
        return nil
    end

    local origin_x = self.x + self.width / 2
    local origin_y = self.y + self.height / 2
    local best_anchor = nil
    local best_score = math.huge

    for _, anchor in ipairs(level.grapple_anchors) do
        local dx = anchor.x - origin_x
        local dy = anchor.y - origin_y
        local distance = math.sqrt(dx * dx + dy * dy)
        local forward = (self.facing == 1 and dx >= -12) or (self.facing == -1 and dx <= 12)
        local upward = dy <= 24
        if forward and upward and distance <= GRAPPLE_MAX_LENGTH and has_line_of_sight(level, origin_x, origin_y, anchor.x, anchor.y) then
            local score = distance + math.abs(dx) * 0.15 + math.max(0, dy) * 0.3
            if score < best_score then
                best_score = score
                best_anchor = anchor
            end
        end
    end

    return best_anchor
end

function Player:fire_grapple_auto()
    local origin_x = self.x + self.width / 2
    local origin_y = self.y + self.height / 2
    local target_x = origin_x + self.facing * 220
    local target_y = origin_y - 120
    local anchor = self:find_best_grapple_anchor(target_x, target_y) or self:find_fallback_grapple_anchor()
    if anchor then
        self:fire_grapple(anchor.x, anchor.y)
    else
        self:fire_grapple(target_x, target_y)
    end
end

function Player:reset_grapple()
    self.grapple_state = "ready"
    self.grapple_target = nil
    self.grapple_aim_anchor = nil
end

function Player:collect_fragment()
    self.fragments = self.fragments + 1
    local camera = get_camera(self)
    local ui = get_ui(self)
    local level = get_level(self)
    if camera then
        camera:shake(2, 0.08)
    end
    if ui and level and level.fragments_required > 0 then
        ui:show_banner(
            level.scene_title,
            string.format("Fragment %d/%d", self.fragments, level.fragments_required),
            level.accent_color,
            1.2
        )
    end
end

return Player
