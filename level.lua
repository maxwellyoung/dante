Level = {}
Level.__index = Level

function Level:new()
    local self = setmetatable({}, Level)
    self.platforms = {
        -- x, y, width, height
        {0, 550, 800, 50},   -- Floor
        {100, 350, 50, 200}, -- Left Wall
        {650, 350, 50, 200}  -- Right Wall
    }
    return self
end

function Level:draw()
    for _, p in ipairs(self.platforms) do
        love.graphics.rectangle("fill", p[1], p[2], p[3], p[4])
    end
end

-- AABB collision check
local function check_collision(a, b)
    return a.x < b.x + b.width and
           a.x + a.width > b.x and
           a.y < b.y + b.height and
           a.y + a.height > b.y
end

function Level:handle_collisions(player)
    player.is_grounded = false
    player.on_wall = 0 -- -1 for left wall, 1 for right wall, 0 for none

    for _, p_rect in ipairs(self.platforms) do
        local platform = {
            x = p_rect[1], y = p_rect[2],
            width = p_rect[3], height = p_rect[4]
        }
        
        if check_collision(player, platform) then
            -- Get center points
            local player_center_x = player.x + player.width / 2
            local player_center_y = player.y + player.height / 2
            local platform_center_x = platform.x + platform.width / 2
            local platform_center_y = platform.y + platform.height / 2
            
            -- Calculate overlap
            local overlap_x = (player.width / 2 + platform.width / 2) - math.abs(player_center_x - platform_center_x)
            local overlap_y = (player.height / 2 + platform.height / 2) - math.abs(player_center_y - platform_center_y)

            if overlap_x < overlap_y then
                -- Horizontal collision
                if player_center_x < platform_center_x then -- Player is to the left
                    player.x = platform.x - player.width
                    player.on_wall = -1
                else -- Player is to the right
                    player.x = platform.x + platform.width
                    player.on_wall = 1
                end
                if player.is_dashing then player.is_dashing = false; player.vx = 0 end
            else
                -- Vertical collision
                if player_center_y < platform_center_y then -- Player is above
                    player.y = platform.y - player.height
                    if player.vy >= 0 then
                        player.vy = 0
                        player.is_grounded = true
                        player.jumps_made = 0
                        if player.is_dashing then player.is_dashing = false; player.vy = 0 end
                    end
                else -- Player is below
                    player.y = platform.y + platform.height
                    player.vy = 0
                end
            end
        end
    end
end

return Level