local Utils = require("utils")

local Collectibles = {}
Collectibles.__index = Collectibles

local COLLECTIBLE_SIZE = 16

function Collectibles.new()
    local instance = setmetatable({}, Collectibles)
    instance.items = {}
    instance.active_items = {}
    return instance
end

function Collectibles:add(x, y)
    local item = {
        x = x,
        y = y,
        width = COLLECTIBLE_SIZE,
        height = COLLECTIBLE_SIZE,
        initial_y = y,
        angle = math.random() * math.pi * 2,
    }
    table.insert(self.items, item)
    table.insert(self.active_items, item)
end

function Collectibles:update(dt, player)
    for i = #self.active_items, 1, -1 do
        local item = self.active_items[i]
        item.angle = item.angle + 3 * dt
        item.y = item.initial_y + math.sin(item.angle) * 5

        if Utils.check_collision(player, item) then
            g_player:collect_fragment()
            g_sfx:play("collect")
            table.remove(self.active_items, i)
        end
    end
end

function Collectibles:draw()
    for _, item in ipairs(self.active_items) do
        local center_x = item.x + item.width / 2
        local center_y = item.y + item.height / 2
        local glow = 0.18 + 0.06 * math.sin(item.angle * 2)

        love.graphics.setColor(1, 0.82, 0.28, glow)
        love.graphics.circle("fill", center_x, center_y, 12)
        love.graphics.setColor(1, 0.9, 0.42, 1)
        love.graphics.polygon(
            "fill",
            center_x,
            center_y - 8,
            center_x + 6,
            center_y,
            center_x,
            center_y + 8,
            center_x - 6,
            center_y
        )
        love.graphics.setColor(1, 1, 0.88, 0.85)
        love.graphics.polygon(
            "line",
            center_x,
            center_y - 8,
            center_x + 6,
            center_y,
            center_x,
            center_y + 8,
            center_x - 6,
            center_y
        )
    end
    love.graphics.setColor(1, 1, 1, 1)
end

function Collectibles:clear()
    self.items = {}
    self.active_items = {}
end

function Collectibles:respawn()
    self.active_items = {}
    for _, item in ipairs(self.items) do
        table.insert(self.active_items, item)
    end
end

return Collectibles
