Player = {}
Player.__index = Player

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
    self.height = 64
    self.vx = 0
    self.vy = 0
    self.is_grounded = false
    
    -- Abilities
    self.has_double_jump = true
    self.jumps_made = 0
    self.has_dash = true
    self.is_dashing = false
    self.dash_timer = 0
    self.dash_direction = {x = 0, y = 0}
    self.has_wall_cling = true
    self.is_wall_sliding = false
    self.on_wall = 0 -- Will be updated by level collision handler
    self.coyote_timer = 0
    self.jump_buffer_timer = 0

    return self
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
    end

    -- Update position
    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt
end

function Player:jump(force)
    force = force or false
    if not force and self.jump_buffer_timer <= 0 then
        self.jump_buffer_timer = JUMP_BUFFER_TIME
        return
    end

    if self.is_wall_sliding then
        self.vy = JUMP_FORCE
        self.vx = -self.on_wall * MOVE_SPEED * 1.2 -- Wall jump away with extra force
        self.is_wall_sliding = false
        self.jumps_made = 1
    elseif self.coyote_timer > 0 then
        self.vy = JUMP_FORCE
        self.is_grounded = false
        self.coyote_timer = 0 -- Consume coyote time
        self.jumps_made = 1
    elseif self.has_double_jump and self.jumps_made < 2 then
        self.vy = JUMP_FORCE
        self.jumps_made = self.jumps_made + 1
    end
end

function Player:dash()
    if not self.has_dash or self.is_dashing then return end

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
    love.graphics.rectangle("fill", self.x, self.y, self.width, self.height)
end

return Player 