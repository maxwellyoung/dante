-- ui.lua
-- Dante's Inferno themed UI with Vlambeer-style juice

local UI = {}
UI.__index = UI

-- The Nine Circles of Hell
local CIRCLES = {
    [0] = { name = "LIMBO", color = {0.5, 0.5, 0.6}, desc = "The Unbaptized" },
    [1] = { name = "LUST", color = {0.9, 0.3, 0.5}, desc = "Slaves to Desire" },
    [2] = { name = "GLUTTONY", color = {0.6, 0.4, 0.2}, desc = "The Insatiable" },
    [3] = { name = "GREED", color = {0.9, 0.7, 0.1}, desc = "Hoarders & Wasters" },
    [4] = { name = "WRATH", color = {0.9, 0.2, 0.1}, desc = "The Wrathful" },
    [5] = { name = "HERESY", color = {0.4, 0.1, 0.4}, desc = "Deniers of Truth" },
    [6] = { name = "VIOLENCE", color = {0.7, 0.1, 0.1}, desc = "Against All" },
    [7] = { name = "FRAUD", color = {0.2, 0.3, 0.5}, desc = "The Deceivers" },
    [8] = { name = "TREACHERY", color = {0.1, 0.2, 0.4}, desc = "Frozen in Ice" },
}

function UI:new()
    local self = setmetatable({}, UI)
    self.time = 0
    self.shake = 0
    self.flash_alpha = 0
    return self
end

function UI:update(dt)
    self.time = self.time + dt
    self.shake = math.max(0, self.shake - dt * 10)
    self.flash_alpha = math.max(0, self.flash_alpha - dt * 3)
end

function UI:trigger_flash()
    self.flash_alpha = 0.3
end

function UI:draw_health(health, max_health)
    local size = 14
    local spacing = 3
    local start_x = g_native_width - 20 - (max_health * (size + spacing))
    local y = 12

    -- Souls/Health label
    love.graphics.setColor(0.6, 0.4, 0.4, 0.8)
    love.graphics.print("VITAE", start_x - 40, y + 2)

    for i = 1, max_health do
        local x = start_x + (i - 1) * (size + spacing)
        local pulse = math.sin(self.time * 3 + i) * 0.1

        if i <= health then
            -- Full soul - burning ember
            love.graphics.setColor(0.9 + pulse, 0.3, 0.1, 1)
            love.graphics.rectangle("fill", x, y, size, size, 3, 3)
            -- Inner glow
            love.graphics.setColor(1, 0.6 + pulse, 0.2, 0.8)
            love.graphics.rectangle("fill", x + 3, y + 3, size - 6, size - 6, 2, 2)
        else
            -- Empty soul - ash
            love.graphics.setColor(0.2, 0.15, 0.15, 0.6)
            love.graphics.rectangle("fill", x, y, size, size, 3, 3)
            love.graphics.setColor(0.3, 0.2, 0.2, 0.3)
            love.graphics.rectangle("line", x, y, size, size, 3, 3)
        end
    end
    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_fragments(collected, required, current_circle)
    local circle_info = CIRCLES[current_circle] or CIRCLES[0]
    local progress = math.min(1, collected / required)
    local pulse = math.sin(self.time * 2) * 0.1 + 0.9

    -- Main panel - dark infernal style
    local panel_x, panel_y = 12, 8
    local panel_w, panel_h = 180, 55

    -- Panel shadow
    love.graphics.setColor(0, 0, 0, 0.5)
    love.graphics.rectangle("fill", panel_x + 2, panel_y + 2, panel_w, panel_h, 4, 4)

    -- Panel background with circle color tint
    local cc = circle_info.color
    love.graphics.setColor(cc[1] * 0.15, cc[2] * 0.15, cc[3] * 0.15, 0.9)
    love.graphics.rectangle("fill", panel_x, panel_y, panel_w, panel_h, 4, 4)

    -- Border with circle color
    love.graphics.setColor(cc[1] * 0.6, cc[2] * 0.6, cc[3] * 0.6, 0.8 * pulse)
    love.graphics.setLineWidth(2)
    love.graphics.rectangle("line", panel_x, panel_y, panel_w, panel_h, 4, 4)
    love.graphics.setLineWidth(1)

    -- Circle name (big, prominent)
    love.graphics.setColor(cc[1], cc[2], cc[3], 1)
    love.graphics.print(circle_info.name, panel_x + 8, panel_y + 6)

    -- Circle description
    love.graphics.setColor(0.6, 0.5, 0.5, 0.7)
    love.graphics.print(circle_info.desc, panel_x + 8, panel_y + 20)

    -- Progress bar background
    local bar_x = panel_x + 8
    local bar_y = panel_y + 38
    local bar_w = panel_w - 16
    local bar_h = 8

    love.graphics.setColor(0.1, 0.08, 0.08, 0.8)
    love.graphics.rectangle("fill", bar_x, bar_y, bar_w, bar_h, 2, 2)

    -- Progress fill with fire gradient
    if progress > 0 then
        local fill_w = bar_w * progress

        -- Base fill
        love.graphics.setColor(cc[1] * 0.8, cc[2] * 0.5, cc[3] * 0.3, 1)
        love.graphics.rectangle("fill", bar_x, bar_y, fill_w, bar_h, 2, 2)

        -- Hot core
        love.graphics.setColor(cc[1], cc[2] * 0.8 + 0.2, cc[3] * 0.5, 0.8)
        love.graphics.rectangle("fill", bar_x, bar_y, fill_w, bar_h / 2, 2, 2)

        -- Completion glow
        if progress >= 1 then
            love.graphics.setColor(1, 0.8, 0.4, 0.4 * pulse)
            love.graphics.rectangle("fill", bar_x - 2, bar_y - 2, fill_w + 4, bar_h + 4, 3, 3)
        end
    end

    -- Fragment count
    love.graphics.setColor(0.9, 0.8, 0.7, 1)
    local count_text = string.format("%d/%d", collected, required)
    love.graphics.print(count_text, panel_x + panel_w - 35, panel_y + 36)

    -- Screen flash overlay
    if self.flash_alpha > 0 then
        love.graphics.setColor(1, 0.8, 0.5, self.flash_alpha)
        love.graphics.rectangle("fill", 0, 0, g_native_width, g_native_height)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_death_screen()
    -- Dark overlay
    love.graphics.setColor(0, 0, 0, 0.7)
    love.graphics.rectangle("fill", 0, 0, g_native_width, g_native_height)

    -- Blood red vignette
    local pulse = math.sin(self.time * 2) * 0.1 + 0.5
    love.graphics.setColor(0.5, 0, 0, pulse * 0.3)
    love.graphics.rectangle("fill", 0, 0, g_native_width, 40)
    love.graphics.rectangle("fill", 0, g_native_height - 40, g_native_width, 40)

    -- Death text
    love.graphics.setColor(0.8, 0.2, 0.1, 1)
    love.graphics.printf("DAMNED", 0, g_native_height / 2 - 25, g_native_width, "center")

    love.graphics.setColor(0.5, 0.4, 0.4, 0.8)
    love.graphics.printf("Returning to checkpoint...", 0, g_native_height / 2 + 5, g_native_width, "center")

    love.graphics.setColor(1, 1, 1, 1)
end

return UI
