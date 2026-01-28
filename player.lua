-- player.lua
-- Contains all logic for the player character.

local Cooldown = require('cooldown')
local Player = {}
Player.__index = Player

local TILE_SIZE = 32
local RUN_SPEED = 400
local WALK_SPEED = 280
local GRAPPLE_SPEED = 1200
local GRAPPLE_MAX_LENGTH = 350
-- Sprite dimensions - will be calculated from sheet
local SPRITE_WIDTH, SPRITE_HEIGHT = 170, 170 -- ~1024/6 for 3x2 grid

function Player:new()
    local self = setmetatable({}, { __index = Player })
    self.x, self.y = 100, 100
    self.width, self.height = 28, 28
    self.vx, self.vy = 0, 0
    
    self.jump_force = -550
    self.gravity = 1600
    self.on_ground = false
    self.was_on_ground = false -- Track previous frame for landing detection
    self.on_wall = 0 -- -1 for left wall, 1 for right wall, 0 for none
    self.wall_slide_speed = 100 -- Max fall speed when wall sliding
    self.wall_jump_force_x = 300
    self.wall_jump_force_y = -500
    self.facing = 1
    self.fragments = 0

    -- Health system
    self.max_health = 3
    self.health = self.max_health
    self.invincible_timer = 0
    self.invincible_duration = 1.5 -- Seconds of invincibility after being hit
    self.is_dead = false
    
    self.shoot_cd = Cooldown:new(0.2)
    self.grapple_cd = Cooldown:new(1.5)
    
    self.grapple_state = "ready"
    self.grapple_hook = { x=0, y=0, vx=0, vy=0 }
    self.grapple_target = nil
    
    -- NEW Animation System
    self.sprite_sheet = love.graphics.newImage("player_sprites.png")
    self.sprite_sheet:setFilter("nearest", "nearest")
    self.quads = {}
    self.anim_timer = 0
    self.anim_frame = 1
    self.anim_state = "idle"
    self:load_quads()

    return self
end

function Player:load_quads()
    local sw, sh = self.sprite_sheet:getDimensions()
    -- Auto-detect sprite size: assume 3 columns, 2 rows based on the sprite sheet
    local cols, rows = 3, 2
    SPRITE_WIDTH = math.floor(sw / cols)
    SPRITE_HEIGHT = math.floor(sh / rows)

    -- All 6 frames work as idle/run animation
    self.quads.idle = {}
    self.quads.run = {}
    self.quads.jump = {}
    self.quads.fall = {}

    -- Row 0: first 3 frames
    for i = 0, cols - 1 do
        table.insert(self.quads.idle, love.graphics.newQuad(i * SPRITE_WIDTH, 0, SPRITE_WIDTH, SPRITE_HEIGHT, sw, sh))
        table.insert(self.quads.run, love.graphics.newQuad(i * SPRITE_WIDTH, 0, SPRITE_WIDTH, SPRITE_HEIGHT, sw, sh))
    end
    -- Row 1: next 3 frames (add to run for more frames)
    for i = 0, cols - 1 do
        table.insert(self.quads.run, love.graphics.newQuad(i * SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_WIDTH, SPRITE_HEIGHT, sw, sh))
    end

    -- Use first frame for jump/fall if no dedicated sprites
    self.quads.jump = { self.quads.idle[1] }
    self.quads.fall = { self.quads.idle[1] }
end

function Player:set_start_pos(pos)
    self.x, self.y = pos.x, pos.y
end

function Player:update(dt, level)
    local current_speed = love.keyboard.isDown("lshift") and RUN_SPEED or WALK_SPEED
    if self.grapple_state ~= "hooked" then
        if love.keyboard.isDown("a") then
            self.vx = -current_speed
            self.facing = -1
        elseif love.keyboard.isDown("d") then
            self.vx = current_speed
            self.facing = 1
        else
            self.vx = 0
        end
    end

    -- Grapple Logic
    if self.grapple_state == "firing" then
        self:update_grapple_firing(dt, level)
    elseif self.grapple_state == "hooked" then
        self:update_grapple_hooked(dt, level)
    else
        -- Standard gravity
        self.vy = self.vy + self.gravity * dt
        self.vy = math.min(self.vy, 1000)
    end

    -- Standard Collision
    local new_x = self.x + self.vx * dt
    local new_y = self.y + self.vy * dt

    self.was_on_ground = self.on_ground
    self.on_ground = false
    self.on_wall = 0

    -- Vertical collision
    if self.vy > 0 then
        if level:is_solid_at_pixel(self.x, new_y + self.height) or level:is_solid_at_pixel(self.x + self.width, new_y + self.height) then
            new_y = math.floor((new_y + self.height) / TILE_SIZE) * TILE_SIZE - self.height
            -- Landing effects (only if falling fast enough)
            if self.vy > 200 and not self.was_on_ground then
                g_effects:spawn('land_dust', self.x + self.width/2, new_y + self.height)
                g_sfx:play('land')
            end
            self.vy = 0
            self.on_ground = true
        end
    elseif self.vy < 0 then
        if level:is_solid_at_pixel(self.x, new_y) or level:is_solid_at_pixel(self.x + self.width, new_y) then
            new_y = (math.floor(new_y / TILE_SIZE) + 1) * TILE_SIZE
            self.vy = 0
        end
    end

    -- Horizontal collision
    if self.vx > 0 then
        if level:is_solid_at_pixel(new_x + self.width, self.y) or level:is_solid_at_pixel(new_x + self.width, self.y + self.height - 1) then
            new_x = math.floor((new_x + self.width) / TILE_SIZE) * TILE_SIZE - self.width
            self.vx = 0
        end
    elseif self.vx < 0 then
        if level:is_solid_at_pixel(new_x, self.y) or level:is_solid_at_pixel(new_x, self.y + self.height - 1) then
            new_x = (math.floor(new_x / TILE_SIZE) + 1) * TILE_SIZE
            self.vx = 0
        end
    end

    self.x, self.y = new_x, new_y

    -- Wall detection for wall sliding (only when airborne and falling)
    if not self.on_ground and self.vy > 0 then
        local wall_check_y = self.y + self.height / 2
        if level:is_solid_at_pixel(self.x - 1, wall_check_y) then
            self.on_wall = -1
        elseif level:is_solid_at_pixel(self.x + self.width + 1, wall_check_y) then
            self.on_wall = 1
        end

        -- Apply wall slide (slow down fall when against wall and holding toward it)
        if self.on_wall ~= 0 then
            local holding_toward_wall = (self.on_wall == -1 and love.keyboard.isDown("a")) or
                                        (self.on_wall == 1 and love.keyboard.isDown("d"))
            if holding_toward_wall and self.vy > self.wall_slide_speed then
                self.vy = self.wall_slide_speed
                g_effects:spawn('wall_slide_dust', self.x + (self.on_wall == 1 and self.width or 0), self.y + self.height / 2, {dir = -self.on_wall})
            end
        end
    end
    self:update_animation(dt)
    self.shoot_cd:update(dt)
    self.grapple_cd:update(dt)

    -- Update invincibility timer
    if self.invincible_timer > 0 then
        self.invincible_timer = self.invincible_timer - dt
    end
end

function Player:take_damage()
    if self.invincible_timer > 0 or self.is_dead then return false end

    self.health = self.health - 1
    g_sfx:play('hit_enemy')
    g_camera:shake(5, 0.3)
    g_effects:spawn('blood', self.x + self.width/2, self.y + self.height/2)

    if self.health <= 0 then
        self:die()
        return true
    else
        self.invincible_timer = self.invincible_duration
        return false
    end
end

function Player:die()
    self.is_dead = true
    g_sfx:play('death')
    g_effects:spawn('blood', self.x + self.width/2, self.y + self.height/2)
end

function Player:respawn(pos)
    self.x, self.y = pos.x, pos.y
    self.vx, self.vy = 0, 0
    self.health = self.max_health
    self.is_dead = false
    self.invincible_timer = 1.0 -- Brief invincibility after respawn
    self.grapple_state = "ready"
end

function Player:is_invincible()
    return self.invincible_timer > 0
end

function Player:update_animation(dt)
    local old_state = self.anim_state
    
    -- Determine current animation state
    if not self.on_ground then
        if self.on_wall ~= 0 then
            self.anim_state = "fall" -- Could be "wall_slide" if you add that animation
        elseif self.vy < -50 then
            self.anim_state = "jump"
        else
            self.anim_state = "fall"
        end
    elseif self.vx ~= 0 then
        self.anim_state = "run"
    else
        self.anim_state = "idle"
    end

    -- Reset timer if state changed
    if self.anim_state ~= old_state then
        self.anim_timer = 0
        self.anim_frame = 1
    end

    -- Update timer and frame
    self.anim_timer = self.anim_timer + dt
    local rate = 0.1 -- default frame rate
    if self.anim_state == "idle" then rate = 0.2 end
    if self.anim_state == "run" then rate = 0.08 end

    if self.anim_timer > rate then
        self.anim_timer = self.anim_timer - rate
        self.anim_frame = self.anim_frame + 1
        if self.anim_frame > #self.quads[self.anim_state] then
            self.anim_frame = 1
        end
    end
end

function Player:update_grapple_firing(dt, level)
    self.grapple_hook.x = self.grapple_hook.x + self.grapple_hook.vx * dt
    self.grapple_hook.y = self.grapple_hook.y + self.grapple_hook.vy * dt
    
    local dist = math.sqrt((self.grapple_hook.x - self.x)^2 + (self.grapple_hook.y - self.y)^2)
    if dist > GRAPPLE_MAX_LENGTH then
        self:reset_grapple()
    elseif level:is_solid_at_pixel(self.grapple_hook.x, self.grapple_hook.y) then
        self.grapple_state = "hooked"
        self.grapple_target = {x = self.grapple_hook.x, y = self.grapple_hook.y}
    end
end

function Player:update_grapple_hooked(dt, level)
    local angle_to_hook = math.atan2(self.y - self.grapple_target.y, self.x - self.grapple_target.x)
    local dist_to_hook = math.sqrt((self.x - self.grapple_target.x)^2 + (self.y - self.grapple_target.y)^2)

    local tangential_v = self.vx * math.cos(angle_to_hook + math.pi/2) + self.vy * math.sin(angle_to_hook + math.pi/2)
    
    local gravity_effect = self.gravity * math.cos(angle_to_hook) * dt
    tangential_v = tangential_v + gravity_effect
    
    self.vx = tangential_v * math.cos(angle_to_hook + math.pi/2)
    self.vy = tangential_v * math.sin(angle_to_hook + math.pi/2)
    
    local next_x = self.x + self.vx * dt
    local next_y = self.y + self.vy * dt
    local next_dist = math.sqrt((next_x - self.grapple_target.x)^2 + (next_y - self.grapple_target.y)^2)
    
    self.x, self.y = next_x, next_y
end

function Player:keypressed(key)
    if key == "space" then
        if self.on_ground then
            self.vy = self.jump_force
            g_effects:spawn('jump_dust', self.x + self.width/2, self.y + self.height)
            g_sfx:play('jump')
        elseif self.on_wall ~= 0 then
            -- Wall jump: push away from wall and up
            self.vx = self.wall_jump_force_x * (-self.on_wall)
            self.vy = self.wall_jump_force_y
            self.facing = -self.on_wall
            g_effects:spawn('jump_dust', self.x + (self.on_wall == 1 and self.width or 0), self.y + self.height / 2)
            g_sfx:play('jump')
        elseif self.grapple_state == "hooked" then
            self:reset_grapple()
            self.vy = self.jump_force * 0.8
        end
    end
end

function Player:draw()
    -- Draw Grapple Hook
    if self.grapple_state == "firing" or self.grapple_state == "hooked" then
        love.graphics.setColor(0.8, 0.8, 0.8)
        love.graphics.line(self.x + self.width/2, self.y + self.height/2, self.grapple_hook.x, self.grapple_hook.y)
        love.graphics.rectangle("fill", self.grapple_hook.x - 2, self.grapple_hook.y - 2, 4, 4)
    end

    -- Draw Player sprite (flash when invincible)
    if self.invincible_timer > 0 and math.floor(self.invincible_timer * 10) % 2 == 0 then
        love.graphics.setColor(1, 1, 1, 0.5) -- Semi-transparent when flashing
    else
        love.graphics.setColor(1, 1, 1, 1)
    end

    local anim_frames = self.quads[self.anim_state]
    if anim_frames and anim_frames[self.anim_frame] then
        local current_quad = anim_frames[self.anim_frame]
        -- Scale sprite to fit player hitbox
        local target_height = self.height * 1.8
        local scale = target_height / SPRITE_HEIGHT
        local scale_y = scale
        -- Flip horizontally based on facing direction
        local scale_x = scale * self.facing
        local ox = SPRITE_WIDTH / 2
        local oy = SPRITE_HEIGHT
        local draw_x = self.x + self.width / 2
        local draw_y = self.y + self.height
        love.graphics.draw(self.sprite_sheet, current_quad, math.floor(draw_x), math.floor(draw_y), 0, scale_x, scale_y, ox, oy)
    else
        -- Fallback to colored rectangle if sprite not loaded
        love.graphics.setColor(0.8, 0.2, 0.2)
        love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
        love.graphics.setColor(1, 1, 1)
    end
end

function Player:fire_weapon(target_x, target_y)
    if self.shoot_cd:is_ready() then
        self:reset_grapple()
        self.shoot_cd:use()
        g_effects:spawn('muzzle_flash', self.x + self.width/2, self.y + self.height/2, {dir = self.facing})

        local proj_speed = 800
        local angle = math.atan2(target_y - (self.y + self.height/2), target_x - (self.x + self.width/2))
        local vx = math.cos(angle) * proj_speed
        local vy = math.sin(angle) * proj_speed

        g_projectiles:spawn(self.x + self.width/2, self.y + self.height/2, vx, vy)

        -- Vlambeer juice: recoil and screen shake
        self.vx = self.vx - math.cos(angle) * 80
        g_camera:shake(3, 0.1)
        g_ui:trigger_flash()
    end
end

function Player:fire_grapple(target_x, target_y)
    if self.grapple_cd:is_ready() then
        self.grapple_cd:use()
        self.grapple_state = "firing"
        self.grapple_hook.x = self.x + self.width/2
        self.grapple_hook.y = self.y + self.height/2

        local angle = math.atan2(target_y - self.grapple_hook.y, target_x - self.grapple_hook.x)
        self.grapple_hook.vx = math.cos(angle) * GRAPPLE_SPEED
        self.grapple_hook.vy = math.sin(angle) * GRAPPLE_SPEED
        g_sfx:play('grapple')
    end
end

function Player:reset_grapple()
    self.grapple_state = "ready"
end

function Player:collect_fragment()
    self.fragments = self.fragments + 1
end

return Player 