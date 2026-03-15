-- fps_map.lua
-- Grid-based map for the raycaster FPS mode.

local FPSMap = {}
FPSMap.__index = FPSMap

function FPSMap:new(data)
    local instance = setmetatable({}, self)
    instance.grid = {}
    instance.width = 0
    instance.height = 0
    instance.spawn = { x = 2.5, y = 2.5, angle = 0 }
    instance.exit = nil
    instance.sprites = {}
    instance.world_texts = {}
    instance.triggers = {}
    instance:load(data)
    return instance
end

function FPSMap:load(data)
    if not data or not data.grid then return end

    self.height = #data.grid
    self.width = #data.grid[1]
    self.grid = {}
    self.sprites = {}
    self.world_texts = data.world_texts or {}
    self.triggers = data.triggers or {}

    for y, row in ipairs(data.grid) do
        self.grid[y] = {}
        for x = 1, #row do
            local char = row:sub(x, x)
            local tile = 0

            if char == "#" or char == "1" then
                tile = 1  -- stone wall
            elseif char == "2" then
                tile = 2  -- hell wall
            elseif char == "3" then
                tile = 3  -- sludge wall
            elseif char == "P" then
                self.spawn = { x = x - 0.5, y = y - 0.5, angle = data.spawn_angle or 0 }
            elseif char == "X" then
                self.exit = { x = x - 0.5, y = y - 0.5, radius = 0.6 }
            elseif char == "E" then
                table.insert(self.sprites, {
                    x = x - 0.5, y = y - 0.5,
                    type = "enemy",
                    color = { 0.85, 0.25, 0.2 },
                    health = 2,
                    active = true,
                    has_face = true,
                    width_ratio = 0.7,
                    speed = 1.2,
                    attack_range = 1.5,
                    attack_cooldown = 0,
                })
            elseif char == "D" then
                table.insert(self.sprites, {
                    x = x - 0.5, y = y - 0.5,
                    type = "demon",
                    color = { 0.7, 0.15, 0.1 },
                    health = 4,
                    active = true,
                    has_face = true,
                    width_ratio = 1.0,
                    speed = 0.8,
                    attack_range = 2.0,
                    attack_cooldown = 0,
                })
            elseif char == "H" then
                table.insert(self.sprites, {
                    x = x - 0.5, y = y - 0.5,
                    type = "pickup_health",
                    color = { 0.3, 0.9, 0.4 },
                    active = true,
                    has_face = false,
                    width_ratio = 0.5,
                })
            elseif char == "K" then
                table.insert(self.sprites, {
                    x = x - 0.5, y = y - 0.5,
                    type = "pickup_key",
                    color = { 1, 0.85, 0.2 },
                    active = true,
                    has_face = false,
                    width_ratio = 0.4,
                })
            end

            self.grid[y][x] = tile
        end
    end
end

function FPSMap:get(x, y)
    local gy = math.floor(y) + 1
    local gx = math.floor(x) + 1
    if gy < 1 or gy > self.height or gx < 1 or gx > self.width then
        return 1  -- out of bounds = wall
    end
    return self.grid[gy] and self.grid[gy][gx] or 1
end

function FPSMap:is_solid(x, y)
    return self:get(x, y) > 0
end

function FPSMap:get_enemies()
    local enemies = {}
    for _, s in ipairs(self.sprites) do
        if (s.type == "enemy" or s.type == "demon") and s.active then
            table.insert(enemies, s)
        end
    end
    return enemies
end

function FPSMap:enemies_alive()
    for _, s in ipairs(self.sprites) do
        if (s.type == "enemy" or s.type == "demon") and s.active then
            return true
        end
    end
    return false
end

return FPSMap
