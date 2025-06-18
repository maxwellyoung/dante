-- ui.lua

local UI = {}
UI.__index = UI

function UI:new()
    local self = setmetatable({}, UI)
    return self
end

local circle_names = {
    "Paradiso",
    "Limbo",
    "Lust",
    "Gluttony",
    "Greed",
    "Wrath",
    "Heresy",
    "Violence",
    "Fraud",
    "Treachery"
}

function UI:draw()
    local circle_name = circle_names[g_current_circle + 1] or "Unknown"
    
    love.graphics.push()
    love.graphics.setFont(love.graphics.newFont(24))
    
    -- Draw shadow
    love.graphics.setColor(0, 0, 0, 0.5)
    love.graphics.printf(circle_name, 2, 12, 800, "center")

    -- Draw text
    love.graphics.setColor(1, 1, 1, 0.8)
    love.graphics.printf(circle_name, 0, 10, 800, "center")

    love.graphics.pop()
end

return UI 