Level = {}
Level.__index = Level

function Level:new()
    local self = setmetatable({}, Level)
    self.platforms = {
        -- x, y, width, height
        {0, 550, 800, 50}
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
    player.is_grounded = false -- Assume not grounded until a collision proves otherwise
    for _, p_rect in ipairs(self.platforms) do
        local platform = {
            x = p_rect[1], y = p_rect[2],
            width = p_rect[3], height = p_rect[4]
        }

        if check_collision(player, platform) then
            -- For now, just stop the player from falling through
            if player.vy > 0 then
                player.y = platform.y - player.height
                player.vy = 0
                player.is_grounded = true
                player.jumps_made = 0 -- Reset jumps on landing
            end
        end
    end
end

return Level 