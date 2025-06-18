local Animation = require("animation")

local function sign(n)
    return n > 0 and 1 or (n < 0 and -1 or 0)
end

local function move_towards(current, target, max_delta)
    if math.abs(target - current) <= max_delta then
        return target
    else
        return current + sign(target - current) * max_delta
    end
end

local Player = {}
Player.__index = Player

-- Constants
local GRAVITY = 1400
local MOVE_SPEED = 300
local SPRINT_SPEED = 450
local JUMP_FORCE = -550
local DASH_SPEED = 700
local DASH_DURATION = 0.15
local WALL_SLIDE_SPEED = 90
local COYOTE_TIME = 0.1
local JUMP_BUFFER_TIME = 0.1
local ACCELERATION = 3000
local FRICTION = 3500

function Player:new()
    local self = setmetatable({}, Player)
    self.x = 0
    self.y = 0
    self.width = 32
    self.height = 32
    self.vx = 0
    self.vy = 0
    self.is_grounded = false
    
    self.debug_image = love.graphics.newImage("player_sprites.png")

    -- Animations
    self.animations = {
        idle = Animation:new("player_sprites.png", 32, 32, 1, {1}),
        run = Animation:new("player_sprites.png", 32, 32, 0.4, {2, 3, 4}),
        jump = Animation:new("player_sprites.png", 32, 32, 1, {5}),
        fall = Animation:new("player_sprites.png", 32, 32, 1, {6}),
        dash = Animation:new("player_sprites.png", 32, 32, 1, {7}),
        wall_slide = Animation:new("player_sprites.png", 32, 32, 1, {8}),
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

    self:reset_abilities()

    return self
end

function Player:reset_abilities()
    self.has_double_jump = true
    self.has_dash = true
    self.has_wall_cling = true
end

function Player:set_start_pos(pos)
    self.x = pos.x
    self.y = pos.y
    self.vx = 0
    self.vy = 0
end

function Player:update(dt)
    -- Timers
    if self.coyote_timer > 0 then self.coyote_timer = self.coyote_timer - dt end

    if self.is_grounded then
        self.coyote_timer = COYOTE_TIME
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
        local move_input = 0
        if love.keyboard.isDown("left") or love.keyboard.isDown("a") then
            move_input = -1
        elseif love.keyboard.isDown("right") or love.keyboard.isDown("d") then
            move_input = 1
        end

        local speed = MOVE_SPEED
        if love.keyboard.isDown("lshift") or love.keyboard.isDown("rshift") then
            speed = SPRINT_SPEED
        end

        if move_input ~= 0 then
            self.vx = move_towards(self.vx, speed * move_input, ACCELERATION * dt)
        else
            self.vx = move_towards(self.vx, 0, FRICTION * dt)
        end

        -- Wall slide logic
        self.is_wall_sliding = false
        if self.has_wall_cling and self.on_wall ~= 0 and not self.is_grounded and self.vy >= 0 then
            self.is_wall_sliding = true
            self.vy = move_towards(self.vy, WALL_SLIDE_SPEED, GRAVITY * 2 * dt)
            self.jumps_made = 0 -- Reset jumps on wall contact
        elseif not self.is_grounded then
            -- Apply gravity only when airborne
            self.vy = self.vy + GRAVITY * dt
        end

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
end

function Player:jump()
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
        dir_x = self.facing_direction
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
    if self.current_animation and self.current_animation.draw then
        -- The animation system is no longer used for drawing.
        -- We will draw the player procedurally instead.
    else
        love.graphics.setColor(1, 0, 0) -- Bright red for debugging
        love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
    end
    
    -- Procedural Drawing
    local body_color = {1, 1, 1}
    if self.is_dashing then
        body_color = {1, 0.8, 0.2} -- Cooldown/dash indicator
    end

    local face_color = {0.8, 0.8, 0.8}
    love.graphics.setColor(body_color)
    love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
    
    love.graphics.setColor(face_color)
    local face_x = self.x + self.width / 2
    if self.facing_direction == 1 then
        face_x = self.x + self.width - 4
    else
        face_x = self.x
    end
    love.graphics.rectangle("fill", face_x, self.y, 4, self.height)
end

function Player:keypressed(key)
    if key == "space" or key == "up" or key == "w" or key == "k" then
        self:jump()
    elseif key == "z" or key == "x" or key == "c" then
        self:dash()
    end
end

function Player:keyreleased(key)
    if key == "space" or key == "up" or key == "w" or key == "k" then
        if self.vy < 0 then
            self.vy = self.vy * 0.4 -- Cut upward momentum for variable jump height
        end
    end
end

return Player