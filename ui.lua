-- ui.lua
-- Readability-first HUD with authored slice presentation.

local UI = {}
UI.__index = UI

function UI:new(services)
    local class = self or UI
    local instance = setmetatable({}, class)
    instance.services = services
    instance.time = 0
    instance.flash_alpha = 0
    instance.banner = nil
    instance.chapter_card = nil
    instance.story_callout = nil
    instance.pending_story_callout = nil
    instance.objective_timer = 0
    instance.hint_timer = 0
    return instance
end

function UI:update(dt)
    self.time = self.time + dt
    self.flash_alpha = math.max(0, self.flash_alpha - dt * 4)
    self.hint_timer = math.max(0, self.hint_timer - dt)
    self.objective_timer = math.max(0, self.objective_timer - dt)

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
            if self.pending_story_callout then
                self.story_callout = self.pending_story_callout
                self.pending_story_callout = nil
            end
        end
    end

    if self.story_callout then
        self.story_callout.timer = self.story_callout.timer - dt
        if self.story_callout.timer <= 0 then
            self.story_callout = nil
        end
    end
end

function UI:trigger_flash()
    self.flash_alpha = 0.18
end

function UI:show_banner(title, subtitle, accent, timer)
    self.banner = {
        title = title,
        subtitle = subtitle,
        accent = accent or { 0.9, 0.55, 0.18 },
        timer = timer or 2.4,
    }
end

function UI:show_story_callout(title, subtitle, accent, timer)
    local callout = {
        title = title,
        subtitle = subtitle,
        accent = accent or { 0.9, 0.55, 0.18 },
        timer = timer or 2.8,
    }
    if self.chapter_card then
        self.pending_story_callout = callout
        return
    end
    self.story_callout = callout
end

local function get_game_mode(self)
    return self.services and self.services.game_mode or g_game_mode
end

local function get_level(self)
    return self.services and self.services.level or g_level
end

local function get_player(self)
    return self.services and self.services.player or g_player
end

local function get_encounters(self)
    return self.services and self.services.encounters or g_encounters
end

local function get_run_stats(self)
    return self.services and self.services.run_stats or g_run_stats
end

local Palette = require("infernal_ascent_palette")

local function is_trial_intro_window(scene, room_time)
    return scene
        and scene.trial_rule
        and scene.trial_rule ~= ""
        and (room_time or 0) < 2.2
end

function UI:show_scene_intro(scene, room_index, room_count)
    -- Micro-trial rooms: no chapter card, just a fast banner.
    -- Key this off trial_rule so authored campaign rooms can use richer
    -- room_type values like traversal/combat/posture without losing the fast intro.
    if scene.trial_rule and scene.trial_rule ~= "" then
        self.hint_timer = 0
        self.objective_timer = 0
        self.story_callout = nil
        self.pending_story_callout = nil
        self.chapter_card = nil
        -- Trial rule is shown via draw_trial_rule, just show title briefly
        self:show_banner(
            scene.title or "TRIAL",
            scene.subtitle or "",
            scene.accent_color,
            1.2
        )
        return
    end

    local long_form = scene.mode == "campaign"
        and (scene.room_type == "transition" or room_index == 1 or scene.room_type == "gate")

    self.hint_timer = scene.mode == "proving_ground" and 1.4 or 0
    self.objective_timer = scene.mode == "campaign" and 1.15 or 0
    self.story_callout = nil
    self.pending_story_callout = nil

    if scene.mode == "proving_ground" then
        self.banner = nil
        self.chapter_card = nil
        return
    end

    if not long_form then
        self:show_banner(scene.title or "SCENE", scene.subtitle or "", scene.accent_color)
        return
    end

    self.banner = nil
    self.chapter_card = {
        title = scene.title or "SCENE",
        subtitle = scene.subtitle or "",
        beat = scene.transition_beat or "",
        question = scene.dramatic_question or "",
        hook = scene.environment_hook or "",
        accent = scene.accent_color or Palette.ui.action,
        timer = 2.2,
        removed_ability = scene.removed_ability or "none",
        environment_hook = scene.environment_hook or "",
        objective_text = scene.objective_text or "",
        progress = string.format("Room %d / %d", room_index or 1, room_count or 1),
    }
end

function UI:should_hide_standard_hud()
    return get_game_mode(self) == "campaign" and self.chapter_card ~= nil
end

function UI:draw_health(health, max_health)
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

    -- Death counter
    local stats = get_run_stats(self)
    if stats and (stats.room_deaths or 0) > 0 then
        local deaths = stats.room_deaths
        love.graphics.setColor(0.6, 0.25, 0.2, 0.9)
        love.graphics.printf(
            string.format("DEATHS %d", deaths),
            start_x - 42, y + 16, max_health * 16 + 42, "right"
        )
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_fragments(collected, required, label, room_type)
    local level = get_level(self)
    local game_mode = get_game_mode(self)
    local encounters = get_encounters(self)
    local stats = get_run_stats(self) or {}
    local trial_intro_active = is_trial_intro_window(level and level.scene, stats.room_time)
    local has_compact_objective = level and level.hud_objective_text and level.hud_objective_text ~= ""
    local show_objective = game_mode == "proving_ground"
        or (self.objective_timer > 0 and not trial_intro_active)
        or has_compact_objective
        or ((encounters and encounters:is_active()) and not trial_intro_active)
    local objective_y = 46
    local header_height = show_objective and 58 or 36

    if game_mode == "campaign" and encounters and encounters:is_active() and not trial_intro_active then
        header_height = 66
    end

    love.graphics.setColor(0.03, 0.03, 0.05, game_mode == "campaign" and 0.86 or 0.74)
    love.graphics.rectangle("fill", 0, 0, g_native_width, header_height)
    love.graphics.setColor(0.9, 0.92, 0.96, 1)
    love.graphics.print(label or "SCENE", 12, 10)

    if game_mode == "proving_ground" and room_type then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.print(string.upper(room_type), 12, 28)
    end

    if required > 0 then
        love.graphics.setColor(0.75, 0.78, 0.82, 1)
        love.graphics.print(string.format("Fragments %d/%d", collected, required), 12, 28)
    end

    if game_mode == "campaign" and encounters and encounters:is_active() and not trial_intro_active then
        love.graphics.setColor(0.96, 0.58, 0.34, 1)
        love.graphics.print("Guardian trial active", 12, 46)
        objective_y = 58
    end

    if show_objective then
        local objective_text = nil
        if game_mode == "campaign" and self.objective_timer <= 0 then
            if encounters and encounters:is_active() then
                objective_text = "Clear the trial. Exit right when the gate yields."
            elseif has_compact_objective then
                objective_text = level.hud_objective_text
            elseif required > 0 and collected < required then
                objective_text = string.format(
                    "Collect %d more fragment%s. Exit right.",
                    required - collected,
                    (required - collected) == 1 and "" or "s"
                )
            else
                objective_text = "Exit right."
            end
        end

        love.graphics.setColor(0.72, 0.76, 0.82, 1)
        if level and level.show_gate then
            if objective_text then
                love.graphics.printf(objective_text, 12, objective_y, g_native_width - 24, "left")
            elseif level.hud_objective_text and level.hud_objective_text ~= "" then
                love.graphics.printf(level.hud_objective_text, 12, objective_y, g_native_width - 24, "left")
            elseif level.objective_text and level.objective_text ~= "" then
                love.graphics.printf(level.objective_text, 12, objective_y, g_native_width - 24, "left")
            elseif required > 0 and collected < required then
                love.graphics.printf(
                    string.format(
                        "Collect %d more fragment%s, then move through the EXIT gate.",
                        required - collected,
                        (required - collected) == 1 and "" or "s"
                    ),
                    12,
                    objective_y,
                    g_native_width - 24,
                    "left"
                )
            else
                love.graphics.printf("Move through the EXIT gate on the right.", 12, objective_y, g_native_width - 24, "left")
            end
        elseif game_mode == "proving_ground" then
            love.graphics.printf("Reach the glowing EXIT gate on the right.", 12, objective_y, g_native_width - 24, "left")
        end
    end

    if self.hint_timer > 0 and game_mode == "proving_ground" and not g_autoplay and not g_qa_capture then
        local alpha = math.min(1, self.hint_timer / 0.8)
        love.graphics.setColor(0.92, 0.55, 0.18, alpha)
        love.graphics.print("Space jump  S crouch  C/Ctrl roll  LShift dash  E or RMB grapple", 12, 64)

        love.graphics.setColor(0.72, 0.76, 0.82, alpha)
        love.graphics.print("Tab reset  [ prev  ] next  F3 metrics", 12, 80)
    end

    if game_mode == "proving_ground"
        and level
        and level.recovery_hint
        and level.recovery_hint ~= ""
        and ((stats.room_deaths or 0) > 0 or (stats.room_respawns or 0) > 0 or (stats.resets or 0) > 0) then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.printf(level.recovery_hint, 12, header_height + 4, g_native_width - 24, "left")
    end

    if self.flash_alpha > 0 then
        love.graphics.setColor(1, 0.82, 0.5, self.flash_alpha)
        love.graphics.rectangle("fill", 0, 0, g_native_width, g_native_height)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_campaign_progress(self, room_count, room_index)
    if get_game_mode(self) ~= "campaign" or not room_count then
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

function UI.draw_encounter_status(self)
    local game_mode = get_game_mode(self)
    local encounters = get_encounters(self)
    local level = get_level(self)
    local stats = get_run_stats(self) or {}
    if game_mode ~= "campaign" or not encounters then
        return
    end
    if is_trial_intro_window(level and level.scene, stats.room_time) then
        return
    end

    local progress = encounters:get_progress()
    if not progress or not progress.active then
        return
    end

    local width = 156
    local x = g_native_width - width - 12
    local y = 30

    love.graphics.setColor(0.04, 0.04, 0.05, 0.9)
    love.graphics.rectangle("fill", x, y, width, 34, 4, 4)
    love.graphics.setColor(0.96, 0.58, 0.34, 1)
    love.graphics.print(
        string.format("TRIAL %d/%d", progress.wave, progress.total_waves),
        x + 8,
        y + 5
    )
    love.graphics.setColor(0.82, 0.84, 0.9, 1)
    love.graphics.print(
        string.format("ENEMIES %d", progress.enemies_remaining),
        x + 76,
        y + 5
    )
    local bar_x = x + 8
    local bar_y = y + 22
    local bar_width = width - 16
    local fill = progress.total_waves > 0 and ((progress.wave - 1) / progress.total_waves) or 0
    love.graphics.setColor(0.14, 0.14, 0.16, 1)
    love.graphics.rectangle("fill", bar_x, bar_y, bar_width, 5, 2, 2)
    love.graphics.setColor(0.96, 0.58, 0.34, 1)
    love.graphics.rectangle("fill", bar_x, bar_y, math.max(8, bar_width * fill), 5, 2, 2)
    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_banner()
    if not self.banner then
        return
    end

    local accent = self.banner.accent
    local width = 276
    local x = math.floor((g_native_width - width) / 2)
    local y = 14

    love.graphics.setColor(0.04, 0.04, 0.05, 0.92)
    love.graphics.rectangle("fill", x, y, width, 30, 4, 4)
    love.graphics.setColor(accent[1], accent[2], accent[3], 1)
    love.graphics.rectangle("fill", x, y, 5, 30)
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.printf(self.banner.title or "", x + 12, y + 5, width - 18, "center")

    if self.banner.subtitle and self.banner.subtitle ~= "" then
        love.graphics.setColor(0.75, 0.78, 0.82, 1)
        love.graphics.printf(self.banner.subtitle, x + 12, y + 16, width - 18, "center")
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_chapter_card()
    if not self.chapter_card then
        return
    end

    local card = self.chapter_card
    local alpha = math.min(1, card.timer / 2.2)
    local x = 54
    local y = 88
    local width = g_native_width - 108
    local height = 78

    love.graphics.setColor(0.02, 0.02, 0.03, 0.84 * alpha)
    love.graphics.rectangle("fill", x, y, width, height, 6, 6)
    love.graphics.setColor(card.accent[1], card.accent[2], card.accent[3], 0.95 * alpha)
    love.graphics.rectangle("fill", x, y, 6, height)

    love.graphics.setColor(1, 1, 1, alpha)
    love.graphics.printf(card.title or "", x + 18, y + 10, width - 36, "center")
    love.graphics.setColor(0.82, 0.84, 0.9, alpha)
    love.graphics.printf(card.subtitle or "", x + 18, y + 28, width - 36, "center")
    if card.question and card.question ~= "" then
        love.graphics.setColor(card.accent[1], card.accent[2], card.accent[3], alpha)
        love.graphics.printf(card.question, x + 18, y + 46, width - 36, "center")
    elseif card.beat and card.beat ~= "" then
        love.graphics.setColor(card.accent[1], card.accent[2], card.accent[3], alpha)
        love.graphics.printf(card.beat, x + 18, y + 46, width - 36, "center")
    end

    if card.removed_ability and card.removed_ability ~= "none" then
        love.graphics.setColor(card.accent[1], card.accent[2], card.accent[3], alpha)
        love.graphics.printf("Loss: " .. card.removed_ability, x + 18, y + 62, width - 36, "center")
    elseif card.hook and card.hook ~= "" then
        love.graphics.setColor(0.82, 0.84, 0.9, alpha)
        love.graphics.printf("Now: " .. card.hook, x + 18, y + 62, width - 36, "center")
    elseif card.progress and card.progress ~= "" then
        love.graphics.setColor(0.82, 0.84, 0.9, alpha)
        love.graphics.printf(card.progress, x + 18, y + 62, width - 36, "center")
    elseif card.objective_text and card.objective_text ~= "" then
        love.graphics.setColor(0.82, 0.84, 0.9, alpha)
        love.graphics.printf(card.objective_text, x + 18, y + 62, width - 36, "center")
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI:draw_story_callout()
    if not self.story_callout then
        return
    end

    local callout = self.story_callout
    local alpha = math.min(1, callout.timer / 0.35)
    local width = 208
    local height = 46
    local x = 14
    local y = g_native_height - height - 22

    love.graphics.setColor(0.03, 0.03, 0.04, 0.88 * alpha)
    love.graphics.rectangle("fill", x, y, width, height, 5, 5)
    love.graphics.setColor(callout.accent[1], callout.accent[2], callout.accent[3], alpha)
    love.graphics.rectangle("fill", x, y, 4, height)
    love.graphics.setColor(1, 1, 1, alpha)
    love.graphics.print(callout.title or "", x + 10, y + 6)
    love.graphics.setColor(0.8, 0.84, 0.92, alpha)
    love.graphics.printf(callout.subtitle or "", x + 10, y + 20, width - 18, "left")
    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_debug(self, level, stats, room_count, room_index, force)
    if not force then
        return
    end

    local x = 12
    local y = g_native_height - 104
    local width = 226
    local height = 126
    local accuracy = 0
    if stats.shots_fired > 0 then
        accuracy = math.floor((stats.shots_hit / stats.shots_fired) * 100 + 0.5)
    end

    love.graphics.setColor(0.04, 0.05, 0.06, 0.88)
    love.graphics.rectangle("fill", x, y, width, height, 4, 4)
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
        string.format(
            "Shots %d  Accuracy %d%%  Banks %d  Rolls %d",
            stats.shots_fired,
            accuracy,
            stats.shot_ricochets or 0,
            stats.rolls or 0
        ),
        x + 8,
        y + 56
    )
    local retry_text = stats.room_last_respawn_latency and string.format("%.2fs", stats.room_last_respawn_latency) or "--"
    love.graphics.print(
        string.format("Saves %d  Retry %s", stats.room_checkpoints or 0, retry_text),
        x + 8,
        y + 72
    )
    local player = get_player(self)
    if player and (player.is_crouching or (player.roll_timer or 0) > 0) then
        love.graphics.print(
            string.format(
                "Posture %s",
                (player.roll_timer or 0) > 0 and "ROLL" or "CROUCH"
            ),
            x + 8,
            y + 88
        )
    end

    if level.removed_ability and level.removed_ability ~= "none" then
        love.graphics.setColor(0.96, 0.45, 0.58, 1)
        love.graphics.print("Loss: " .. level.removed_ability, x + width + 10, y + 6)
        love.graphics.setColor(0.78, 0.82, 0.88, 1)
        love.graphics.print("Hook: " .. (level.environment_hook or ""), x + width + 10, y + 24)
    elseif level.environment_hook and level.environment_hook ~= "" then
        love.graphics.setColor(0.78, 0.82, 0.88, 1)
        love.graphics.print("Hook: " .. level.environment_hook, x + width + 10, y + 6)
    end

    if player and player.environment_label then
        love.graphics.setColor(0.86, 0.92, 1, 1)
        love.graphics.print("Zone: " .. player.environment_label, x + width + 10, y + 42)
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
        love.graphics.print(
            string.format(
                "Dash %s  Crouch %s  Roll %s",
                level.abilities.dash ~= false and "ON" or "OFF",
                level.abilities.crouch ~= false and "ON" or "OFF",
                level.abilities.roll ~= false and "ON" or "OFF"
            ),
            x + width + 10,
            y + 76
        )
        love.graphics.print(
            string.format("Ricochet %d", level.weapon_profile and level.weapon_profile.ricochets or 0),
            x + width + 10,
            y + 92
        )
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_trial_timer(_, remaining, total)
    if remaining <= 0 or total <= 0 then return end

    local ratio = remaining / total
    local x = g_native_width / 2
    local y = g_native_height - 28

    -- Timer bar
    local bar_width = 120
    local bar_height = 6
    local bar_x = x - bar_width / 2
    love.graphics.setColor(0.12, 0.12, 0.15, 0.9)
    love.graphics.rectangle("fill", bar_x - 2, y - 2, bar_width + 4, bar_height + 4, 3, 3)

    -- Color shifts from white to red as time runs out
    local r = ratio < 0.3 and 1 or 0.85
    local g = ratio < 0.3 and (0.2 + ratio) or 0.88
    local b = ratio < 0.3 and 0.15 or 0.95
    love.graphics.setColor(r, g, b, 1)
    love.graphics.rectangle("fill", bar_x, y, bar_width * ratio, bar_height, 2, 2)

    -- Seconds text
    local pulse = remaining < 3 and (0.5 + 0.5 * math.sin(love.timer.getTime() * 12)) or 1
    love.graphics.setColor(r, g, b, pulse)
    love.graphics.printf(
        string.format("%.1f", remaining),
        bar_x, y - 14, bar_width, "center"
    )

    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_trial_rule(_, rule, room_time)
    if not rule or rule == "" then return end

    -- Flash the rule for 2 seconds at room start
    local alpha = 0
    if room_time < 0.3 then
        alpha = room_time / 0.3
    elseif room_time < 1.8 then
        alpha = 1
    elseif room_time < 2.2 then
        alpha = 1 - (room_time - 1.8) / 0.4
    end

    if alpha <= 0 then return end

    local width = 260
    local cx = math.floor((g_native_width - width) / 2)
    local cy = 86

    love.graphics.setColor(0.02, 0.02, 0.03, 0.85 * alpha)
    love.graphics.rectangle("fill", cx, cy, width, 24, 4, 4)
    love.graphics.setColor(1, 1, 1, alpha)
    love.graphics.printf(rule, cx, cy + 6, width, "center")
    love.graphics.setColor(1, 1, 1, 1)
end

function UI.draw_circle_score(_, score)
    love.graphics.setColor(0.75, 0.78, 0.85, 0.7)
    love.graphics.printf(
        string.format("%d", score),
        g_native_width - 60, g_native_height - 18, 50, "right"
    )
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
