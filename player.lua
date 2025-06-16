local Animation = require("animation")

local Player = {}
Player.__index = Player

-- Constants
local GRAVITY = 1200
local MOVE_SPEED = 300
local SPRINT_SPEED = 450
local JUMP_FORCE = -500
local DASH_SPEED = 700
local DASH_DURATION = 0.15
local WALL_SLIDE_SPEED = 100
local COYOTE_TIME = 0.1
local JUMP_BUFFER_TIME = 0.1

function Player:new(x, y)
    local self = setmetatable({}, Player)
    self.x = x
    self.y = y
    self.width = 32
    self.height = 32
    self.vx = 0
    self.vy = 0
    self.is_grounded = false
    
    -- Animations
    self.animations = {
        idle = Animation.new("player_sprites.png", 32, 64, 1, {1}),
        run = Animation.new("player_sprites.png", 32, 64, 0.4, {2, 3, 4}),
        jump = Animation.new("player_sprites.png", 32, 64, 1, {5}),
        fall = Animation.new("player_sprites.png", 32, 64, 1, {6}),
        dash = Animation.new("player_sprites.png", 32, 64, 1, {7}),
        wall_slide = Animation.new("player_sprites.png", 32, 64, 1, {8}),
    }
    self.current_animation = self.animations.idle
    self.facing_direction = 1 -- 1 for right, -1 for left

    -- Abilities
    self.jumps_made = 0
    self.is_dashing = false
    self.dash_timer = 0
    self.dash_direction = {x = 0, y = 0}
    self.is_wall_sliding = false
    self.on_wall = 0 -- Will be updated by level collision handler
    self.coyote_timer = 0
    self.jump_buffer_timer = 0

    self:reset_abilities()

    return self
end

function Player:reset_abilities()
    self.has_double_jump = true
    self.has_dash = true
    self.has_wall_cling = true
end

function Player:reset_position(x, y)
    self.x = x
    self.y = y
    self.vx = 0
    self.vy = 0
end

function Player:update(dt)
    -- Timers
    if self.coyote_timer > 0 then self.coyote_timer = self.coyote_timer - dt end
    if self.jump_buffer_timer > 0 then self.jump_buffer_timer = self.jump_buffer_timer - dt end

    if self.is_grounded then
        self.coyote_timer = COYOTE_TIME
    end

    if self.jump_buffer_timer > 0 and self.coyote_timer > 0 then
        self:jump(true) -- Force a jump
        self.jump_buffer_timer = 0
    end
    
    if self.is_dashing then
        -- Dashing logic
        self.dash_timer = self.dash_timer - dt
        if self.dash_timer <= 0 then
            self.is_dashing = false
            self.vx = 0
            self.vy = 0 -- Stop momentum post-dash for Celeste-like control
        else
            self.vx = self.dash_direction.x * DASH_SPEED
            self.vy = self.dash_direction.y * DASH_SPEED
        end
    else
        -- Normal movement logic
        local current_move_speed = MOVE_SPEED
        if love.keyboard.isDown("lshift") or love.keyboard.isDown("rshift") then
            current_move_speed = SPRINT_SPEED
        end

        -- Horizontal movement
        if love.keyboard.isDown("left") or love.keyboard.isDown("a") then
            self.vx = -current_move_speed
        elseif love.keyboard.isDown("right") or love.keyboard.isDown("d") then
            self.vx = current_move_speed
        else
            self.vx = 0
        end

        -- Wall slide logic
        self.is_wall_sliding = false
        if self.has_wall_cling and self.on_wall ~= 0 and not self.is_grounded then
            if self.vy > 0 then -- Only slide down
                self.is_wall_sliding = true
                if self.vy > WALL_SLIDE_SPEED then
                    self.vy = WALL_SLIDE_SPEED
                end
                self.jumps_made = 0 -- Reset jumps on wall contact
            end
        end

        -- Apply gravity
        self.vy = self.vy + GRAVITY * dt

        if self.vx > 0 then
            self.facing_direction = 1
        elseif self.vx < 0 then
            self.facing_direction = -1
        end
    end

    -- Update animation based on state
    if self.is_dashing then
        self.current_animation = self.animations.dash
    elseif self.is_wall_sliding then
        self.current_animation = self.animations.wall_slide
    elseif not self.is_grounded then
        if self.vy < 0 then
            self.current_animation = self.animations.jump
        else
            self.current_animation = self.animations.fall
        end
    elseif self.vx ~= 0 then
        self.current_animation = self.animations.run
    else
        self.current_animation = self.animations.idle
    end
    
    self.current_animation:update(dt)

    -- Update position based on velocity
    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt
end

function Player:jump(force)
    force = force or false
    if not force and self.jump_buffer_timer <= 0 then
        self.jump_buffer_timer = JUMP_BUFFER_TIME
        return
    end

    local did_jump = false
    if self.is_wall_sliding then
        self.vy = JUMP_FORCE
        self.vx = -self.on_wall * MOVE_SPEED * 1.2 -- Wall jump away with extra force
        self.is_wall_sliding = false
        self.jumps_made = 1
        did_jump = true
    elseif self.coyote_timer > 0 then
        self.vy = JUMP_FORCE
        self.is_grounded = false
        self.coyote_timer = 0 -- Consume coyote time
        self.jumps_made = 1
        did_jump = true
    elseif self.has_double_jump and self.jumps_made < 2 then
        self.vy = JUMP_FORCE
        self.jumps_made = self.jumps_made + 1
        did_jump = true
    end

    if did_jump then
        g_effects:jump(self.x + self.width/2, self.y + self.height)
    end
end

function Player:dash()
    if not self.has_dash or self.is_dashing then return end

    g_effects:dash(self.x + self.width/2, self.y + self.height/2)

    self.is_dashing = true
    self.dash_timer = DASH_DURATION

    local dir_x, dir_y = 0, 0
    if love.keyboard.isDown("left") or love.keyboard.isDown("a") then dir_x = -1 end
    if love.keyboard.isDown("right") or love.keyboard.isDown("d") then dir_x = 1 end
    if love.keyboard.isDown("up") or love.keyboard.isDown("w") then dir_y = -1 end
    if love.keyboard.isDown("down") or love.keyboard.isDown("s") then dir_y = 1 end

    -- Default to forward dash if no direction is held
    if dir_x == 0 and dir_y == 0 then
        -- Check player facing direction (simple version)
        if (love.keyboard.isDown("right") or love.keyboard.isDown("d")) then
            dir_x = 1
        else
            dir_x = -1 -- Default to left if not moving right
        end
    end
    
    -- Normalize for diagonal dashing
    if dir_x ~= 0 and dir_y ~= 0 then
        local len = math.sqrt(dir_x^2 + dir_y^2)
        dir_x = dir_x / len
        dir_y = dir_y / len
    end

    self.dash_direction = {x = dir_x, y = dir_y}
    -- Refresh jumps on dash, a-la Celeste
    self.jumps_made = 0
end

function Player:draw()
    love.graphics.setColor(1, 1, 1) -- Ensure player is not tinted
    
    local ox = self.width / 2
    local oy = self.height / 2

    if self.current_animation and self.current_animation.draw then
        self.current_animation:draw(self.x + ox, self.y + oy, 0, self.facing_direction, 1, ox, oy)
    else
        love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
    end
end

return Player