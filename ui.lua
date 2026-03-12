-- ui.lua
-- Readability-first HUD with authored slice presentation.

local UI = {}
UI.__index = UI

function UI:new()
    local class = self or UI
    local instance = setmetatable({}, class)
    instance.time = 0
    instance.flash_alpha = 0
    instance.banner = nil
    instance.chapter_card = nil
    return instance
end

function UI:update(dt)
    self.time = self.time + dt
    self.flash_alpha = math.max(0, self.flash_alpha - dt * 4)

    if self.banner then
        self.banner.timer = self.banner.timer - dt
        if self.banner.timer <= 0 then
            self.banner = nil
        end
    end

    if self.chapter_card then
        self.chapter_card.timer = self.chapter_card.timer - dt
        if self.chapter_card.timer <= 0 then
            self.chapter_card = nil
        end
    end
end

function UI:trigger_flash()
    self.flash_alpha = 0.18
end

function UI:show_banner(title, subtitle, accent)
    self.banner = {
        title = title,
        subtitle = subtitle,
        accent = accent or { 0.9, 0.55, 0.18 },
        timer = 2.4,
    }
end

function UI:show_scene_intro(scene, room_index, room_count)
    local long_form = scene.mode == "campaign"
        and (scene.room_type == "transition" or room_index == 1 or scene.room_type == "gate")

    self:show_banner(scene.title or "SCENE", scene.subtitle or "", scene.accent_color)

    if not long_form then
        return
    end

    self.chapter_card = {
        title = scene.title or "SCENE",
        subtitle = scene.subtitle or "",
        accent = scene.accent_color or { 0.9, 0.55, 0.18 },
        timer = 3.2,
        removed_ability = scene.removed_ability or "none",
        environment_hook = scene.environment_hook or "",
        progress = string.format("Room %d / %d", room_index or 1, room_count or 1),
    }
end

function UI.draw_health(_, health, max_health)
    local start_x = g_native_width - 16 - (max_health * 16)
    local y = 12

    love.graphics.setColor(0.92, 0.55, 0.18, 1)
    love.graphics.print("VITAE", start_x - 42, y + 2)

    for index = 1, max_health do
        local x = start_x + (index - 1) * 16
        love.graphics.setColor(0.18, 0.18, 0.22, 1)
        love.graphics.rectangle("fill", x, y, 12, 12, 2, 2)
        if index <= health then
            love.graphics.setColor(0.93, 0.35, 0.24, 1)
            love.graphics.rectangle("fill", x + 1, y + 1, 10, 10, 2, 2)
        else
            love.graphics.setColor(0.35, 0.35, 0.4, 1)
            love.graphics.rectangle("line", x + 1, y + 1, 10, 10, 2, 2)
        end
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_fragments(collected, required, label)
    love.graphics.setColor(0.9, 0.92, 0.96, 1)
    love.graphics.print(label or "SCENE", 12, 10)

    if required > 0 then
        love.graphics.setColor(0.75, 0.78, 0.82, 1)
        love.graphics.print(string.format("Fragments %d/%d", collected, required), 12, 28)
    end

    if g_game_mode == "proving_ground" then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.print("Tab: reset  [: prev room  ]: next room", 12, 46)
    elseif g_game_mode == "campaign" then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.print("Tab: reset room", 12, 46)
    end

    love.graphics.setColor(0.72, 0.76, 0.82, 1)
    love.graphics.print("F3: metrics overlay", 12, 64)

    if self.flash_alpha > 0 then
        love.graphics.setColor(1, 0.82, 0.5, self.flash_alpha)
        love.graphics.rectangle("fill", 0, 0, g_native_width, g_native_height)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_campaign_progress(_self, room_count, room_index)
    if g_game_mode ~= "campaign" or not room_count then
        return
    end

    local dot_size = 10
    local gap = 6
    local width = room_count * dot_size + (room_count - 1) * gap
    local start_x = math.floor((g_native_width - width) / 2)
    local y = g_native_height - 18

    for index = 1, room_count do
        local x = start_x + (index - 1) * (dot_size + gap)
        love.graphics.setColor(0.12, 0.12, 0.15, 0.9)
        love.graphics.rectangle("fill", x, y, dot_size, 4, 2, 2)
        if index < room_index then
            love.graphics.setColor(0.74, 0.74, 0.82, 1)
            love.graphics.rectangle("fill", x, y, dot_size, 4, 2, 2)
        elseif index == room_index then
            love.graphics.setColor(0.95, 0.55, 0.32, 1)
            love.graphics.rectangle("fill", x, y, dot_size, 4, 2, 2)
        end
    end
    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_banner()
    if not self.banner then
        return
    end

    local accent = self.banner.accent
    local width = 320
    local x = math.floor((g_native_width - width) / 2)
    local y = 14

    love.graphics.setColor(0.04, 0.04, 0.05, 0.92)
    love.graphics.rectangle("fill", x, y, width, 36, 4, 4)
    love.graphics.setColor(accent[1], accent[2], accent[3], 1)
    love.graphics.rectangle("fill", x, y, 5, 36)
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.printf(self.banner.title or "", x + 12, y + 5, width - 18, "center")

    if self.banner.subtitle and self.banner.subtitle ~= "" then
        love.graphics.setColor(0.75, 0.78, 0.82, 1)
        love.graphics.printf(self.banner.subtitle, x + 12, y + 19, width - 18, "center")
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_chapter_card()
    if not self.chapter_card then
        return
    end

    local card = self.chapter_card
    local alpha = math.min(1, card.timer / 3.2)
    local x = 44
    local y = 82
    local width = g_native_width - 88
    local height = 92

    love.graphics.setColor(0.02, 0.02, 0.03, 0.84 * alpha)
    love.graphics.rectangle("fill", x, y, width, height, 6, 6)
    love.graphics.setColor(card.accent[1], card.accent[2], card.accent[3], 0.95 * alpha)
    love.graphics.rectangle("fill", x, y, 6, height)

    love.graphics.setColor(1, 1, 1, alpha)
    love.graphics.printf(card.title or "", x + 18, y + 10, width - 36, "center")
    love.graphics.setColor(0.82, 0.84, 0.9, alpha)
    love.graphics.printf(card.subtitle or "", x + 18, y + 28, width - 36, "center")
    love.graphics.printf(card.progress or "", x + 18, y + 46, width - 36, "center")

    if card.removed_ability and card.removed_ability ~= "none" then
        love.graphics.setColor(card.accent[1], card.accent[2], card.accent[3], alpha)
        love.graphics.printf("Loss: " .. card.removed_ability, x + 18, y + 62, width - 36, "center")
        love.graphics.setColor(0.82, 0.84, 0.9, alpha)
        love.graphics.printf(
            "Hook: " .. (card.environment_hook or ""),
            x + 18,
            y + 76,
            width - 36,
            "center"
        )
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_debug(_self, level, stats, room_count, room_index, force)
    if not force then
        return
    end

    local x = 12
    local y = g_native_height - 88
    local width = 226
    local accuracy = 0
    if stats.shots_fired > 0 then
        accuracy = math.floor((stats.shots_hit / stats.shots_fired) * 100 + 0.5)
    end

    love.graphics.setColor(0.04, 0.05, 0.06, 0.88)
    love.graphics.rectangle("fill", x, y, width, 78, 4, 4)
    love.graphics.setColor(0.92, 0.55, 0.18, 1)
    love.graphics.print(level.room_type or "room", x + 8, y + 6)
    love.graphics.setColor(0.78, 0.82, 0.88, 1)
    love.graphics.print(
        string.format("Room %d/%d", room_index or 1, room_count or 1),
        x + 98,
        y + 6
    )
    love.graphics.print(
        string.format("Room %.1fs  Total %.1fs", stats.room_time, stats.total_time),
        x + 8,
        y + 24
    )
    love.graphics.print(
        string.format("Deaths %d  Resets %d", stats.deaths, stats.resets),
        x + 8,
        y + 40
    )
    love.graphics.print(
        string.format("Shots %d  Accuracy %d%%", stats.shots_fired, accuracy),
        x + 8,
        y + 56
    )

    if level.removed_ability and level.removed_ability ~= "none" then
        love.graphics.setColor(0.96, 0.45, 0.58, 1)
        love.graphics.print("Loss: " .. level.removed_ability, x + width + 10, y + 6)
        love.graphics.setColor(0.78, 0.82, 0.88, 1)
        love.graphics.print("Hook: " .. (level.environment_hook or ""), x + width + 10, y + 24)
    elseif level.environment_hook and level.environment_hook ~= "" then
        love.graphics.setColor(0.78, 0.82, 0.88, 1)
        love.graphics.print("Hook: " .. level.environment_hook, x + width + 10, y + 6)
    end

    if g_player and g_player.environment_label then
        love.graphics.setColor(0.86, 0.92, 1, 1)
        love.graphics.print("Zone: " .. g_player.environment_label, x + width + 10, y + 42)
    end

    if level.abilities then
        love.graphics.setColor(0.82, 0.84, 0.9, 1)
        love.graphics.print(
            string.format(
                "Shoot %s  Grapple %s",
                level.abilities.shoot ~= false and "ON" or "OFF",
                level.abilities.grapple ~= false and "ON" or "OFF"
            ),
            x + width + 10,
            y + 60
        )
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_death_screen(_self)
    love.graphics.setColor(0, 0, 0, 0.55)
    love.graphics.rectangle("fill", 0, 0, g_native_width, g_native_height)

    love.graphics.setColor(0.92, 0.3, 0.22, 1)
    love.graphics.printf("DAMNED", 0, g_native_height / 2 - 18, g_native_width, "center")
    love.graphics.setColor(0.82, 0.84, 0.9, 1)
    love.graphics.printf("Resetting...", 0, g_native_height / 2 + 4, g_native_width, "center")
    love.graphics.setColor(1, 1, 1, 1)
end

return UI
