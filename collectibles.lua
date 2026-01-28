-- collectibles.lua

local Utils = require('utils')

local Collectibles = {}
Collectibles.__index = Collectibles

local COLLECTIBLE_SIZE = 16

function Collectibles:new()
    local self = setmetatable({}, Collectibles)
    self.items = {}
    self.active_items = {}
    return self
end

function Collectibles:add(x, y)
    local item = {
        x = x,
        y = y,
        width = COLLECTIBLE_SIZE,
        height = COLLECTIBLE_SIZE,
        initial_y = y,
        angle = math.random() * math.pi * 2
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
            g_sfx:play('collect')
            table.remove(self.active_items, i)
        end
    end
end

function Collectibles:draw()
    love.graphics.setColor(1, 0.8, 0.2)
    for _, item in ipairs(self.active_items) do
        love.graphics.rectangle("fill", item.x, item.y, item.width, item.height)
    end
    love.graphics.setColor(1,1,1)
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