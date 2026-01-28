-- utils.lua
-- Shared constants and utility functions

local Utils = {}

-- Game constants
Utils.TILE_SIZE = 32
Utils.NATIVE_WIDTH = 480
Utils.NATIVE_HEIGHT = 270

-- Physics constants
Utils.GRAVITY = 1600
Utils.TERMINAL_VELOCITY = 1000
Utils.JUMP_FORCE = -550

-- Player constants
Utils.PLAYER_WIDTH = 28
Utils.PLAYER_HEIGHT = 28
Utils.RUN_SPEED = 400
Utils.WALK_SPEED = 280

-- Grapple constants
Utils.GRAPPLE_SPEED = 1200
Utils.GRAPPLE_MAX_LENGTH = 350

-- AABB collision check (shared utility)
function Utils.check_collision(a, b)
    return a.x < b.x + b.width and
           a.x + a.width > b.x and
           a.y < b.y + b.height and
           a.y + a.height > b.y
end

-- Distance between two points
function Utils.distance(x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    return math.sqrt(dx * dx + dy * dy)
end

-- Clamp a value between min and max
function Utils.clamp(value, min, max)
    return math.max(min, math.min(max, value))
end

-- Linear interpolation
function Utils.lerp(a, b, t)
    return a + (b - a) * t
end

-- Sign of a number (-1, 0, or 1)
function Utils.sign(x)
    if x > 0 then return 1
    elseif x < 0 then return -1
    else return 0 end
end

return Utils
