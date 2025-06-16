-- ui.lua

local UI = {}

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
    love.graphics.setColor(1, 1, 1, 0.8)
    love.graphics.printf(circle_name, 0, 10, 800, "center")
    love.graphics.pop()
end

return UI 