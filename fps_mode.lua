-- fps_mode.lua
-- Lo-fi FPS narrative vignette engine.
-- Blendo Games hard cuts + Beginner's Guide confessional spaces + Kaufman meta-awareness.
-- Each room is a moment, not a level. Walk through. Read. Feel. Cut.

local Raycaster = require("raycaster")
local FPSMap = require("fps_map")
local FPSPlayer = require("fps_player")
local Sfx = require("sfx")

local FPSMode = {}
FPSMode.__index = FPSMode

local RENDER_WIDTH = 320
local RENDER_HEIGHT = 200

function FPSMode:new()
    local instance = setmetatable({}, self)
    instance.canvas = love.graphics.newCanvas(RENDER_WIDTH, RENDER_HEIGHT)
    instance.canvas:setFilter("nearest", "nearest")
    instance.raycaster = Raycaster:new(RENDER_WIDTH, RENDER_HEIGHT)
    instance.player = FPSPlayer:new()
    instance.sfx = Sfx:new()
    instance.map = nil
    instance.time = 0
    instance.room_index = 0
    instance.rooms = {}
    instance.hard_cut_timer = 0
    instance.hard_cut_pending = false
    instance.hard_cut_duration = 0.12
    instance.screen_text = nil
    instance.screen_text_timer = 0
    instance.triggered = {}
    instance.linger_timer = 0
    instance.can_shoot = false
    instance.ambient_source = nil

    love.mouse.setRelativeMode(true)
    love.mouse.setGrabbed(true)

    return instance
end

function FPSMode:load_rooms()
    self.rooms = {
        -- ============================================================
        -- 1. You start in a corridor. There is text on the wall ahead.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.02, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit",
            can_shoot = false,
            grid = {
                "##########",
                "#P.......#",
                "#........#",
                "#........#",
                "#.......X#",
                "##########",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 8.5, y = 3.5, text = "You are descending.", height = 0.5, color = {0.7, 0.72, 0.8} },
            },
            triggers = {
                { x = 5, y = 3, radius = 2, id = "intro_1",
                  screen_text = "You chose to come here.", duration = 3 },
            },
        },

        -- ============================================================
        -- 2. A room that's too big. Your footsteps echo.
        -- The text says something you half-remember.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.03, 0.02, 0.05 },
            floor_color = { 0.08, 0.08, 0.11 },
            ceiling_color = { 0.03, 0.03, 0.05 },
            completion = "exit",
            can_shoot = false,
            linger = 2,
            grid = {
                "##################",
                "#P...............#",
                "#................#",
                "#................#",
                "#......##........#",
                "#......##........#",
                "#................#",
                "#................#",
                "#...............X#",
                "##################",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 7.5, y = 4.0, text = "I made this for you", height = 0.8,
                  color = {0.9, 0.85, 0.75} },
                { x = 7.5, y = 5.5, text = "but I don't remember why", height = 0.3,
                  color = {0.6, 0.58, 0.55} },
            },
            triggers = {
                { x = 10, y = 5, radius = 3, id = "room2_center",
                  screen_text = "The room is bigger than it needs to be.\nThat's the point.", duration = 3.5 },
            },
        },

        -- ============================================================
        -- 3. A corridor that turns. Something red at the end.
        -- First time you can shoot. One enemy. It doesn't move.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.05, 0.02, 0.03 },
            floor_color = { 0.14, 0.08, 0.1 },
            ceiling_color = { 0.06, 0.03, 0.04 },
            completion = "trigger",
            completion_trigger = "shot_it",
            can_shoot = true,
            grid = {
                "##########",
                "#P.......#",
                "#........#",
                "####.....#",
                "   #.....#",
                "   #.....#",
                "   #..E..#",
                "   #.....#",
                "   #######",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 5.5, y = 1.5, text = "Here is a gun.", height = 0.5,
                  color = {0.8, 0.4, 0.35} },
            },
            triggers = {
                { x = 5, y = 2, radius = 1.5, id = "gun_notice",
                  screen_text = "Left click.", duration = 2 },
            },
            on_kill = function(self)
                self.triggered["shot_it"] = true
                self.screen_text = "You did that."
                self.screen_text_timer = 2.5
            end,
        },

        -- ============================================================
        -- 4. You're in a room with no exit. Text appears.
        -- After a moment, the wall opens.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.02, 0.02, 0.03 },
            floor_color = { 0.12, 0.12, 0.15 },
            ceiling_color = { 0.05, 0.05, 0.07 },
            completion = "linger",
            linger = 6,
            can_shoot = false,
            grid = {
                "########",
                "#P.....#",
                "#......#",
                "#......#",
                "#......#",
                "#......#",
                "########",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 4.5, y = 3.5, text = "Wait.", height = 0.6,
                  color = {0.9, 0.9, 0.95} },
            },
            triggers = {
                { x = 4, y = 3, radius = 2, id = "wait_room",
                  screen_text = "There is no exit.\nNot yet.", duration = 3 },
                { x = 4, y = 3, radius = 2, id = "wait_room_2", delay = 4,
                  screen_text = "Okay.", duration = 1.5 },
            },
        },

        -- ============================================================
        -- 5. A long hallway. Text on both walls, overlapping, illegible.
        -- Something is at the end that you can't quite see.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.04, 0.03, 0.06 },
            floor_color = { 0.1, 0.1, 0.14 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit",
            can_shoot = false,
            grid = {
                "######################",
                "#P...................X#",
                "######################",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 4.5, y = 0.8, text = "I keep starting over", height = 0.5,
                  color = {0.5, 0.5, 0.55} },
                { x = 7.5, y = 2.2, text = "each version is worse", height = 0.4,
                  color = {0.45, 0.45, 0.5} },
                { x = 10.5, y = 0.8, text = "but I can't stop", height = 0.5,
                  color = {0.55, 0.52, 0.5} },
                { x = 13.5, y = 2.2, text = "because stopping", height = 0.4,
                  color = {0.5, 0.48, 0.48} },
                { x = 16.5, y = 0.8, text = "would mean", height = 0.5,
                  color = {0.6, 0.55, 0.52} },
                { x = 19.5, y = 1.5, text = "it was finished", height = 0.6,
                  color = {0.8, 0.75, 0.7} },
            },
            triggers = {
                { x = 11, y = 1.5, radius = 2, id = "hallway_mid",
                  screen_text = "The hallway is the game.\nThe game is the hallway.", duration = 3 },
            },
        },

        -- ============================================================
        -- 6. An open room. Enemies that don't attack. They just stand there.
        -- You can shoot them or you can walk past them to the exit.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.06, 0.03, 0.04 },
            floor_color = { 0.15, 0.1, 0.11 },
            ceiling_color = { 0.06, 0.04, 0.05 },
            completion = "exit",
            can_shoot = true,
            grid = {
                "##############",
                "#P...........#",
                "#............#",
                "#....E...E...#",
                "#............#",
                "#..E.....E...#",
                "#............#",
                "#...........X#",
                "##############",
            },
            spawn_angle = 0,
            -- Make enemies non-hostile
            passive_enemies = true,
            world_texts = {
                { x = 7.5, y = 1.2, text = "They won't hurt you.", height = 0.5,
                  color = {0.7, 0.72, 0.8} },
                { x = 7.5, y = 7.8, text = "The exit is behind them.", height = 0.3,
                  color = {0.5, 0.52, 0.55} },
            },
            triggers = {
                { x = 7, y = 4, radius = 3, id = "passive_room",
                  screen_text = "You can shoot them if you want.\nNothing will happen.", duration = 3.5 },
            },
        },

        -- ============================================================
        -- 7. A small room. Just text.
        -- ============================================================
        {
            title = "",
            fog_color = { 0.01, 0.01, 0.02 },
            floor_color = { 0.06, 0.06, 0.08 },
            ceiling_color = { 0.02, 0.02, 0.03 },
            completion = "linger",
            linger = 5,
            can_shoot = false,
            grid = {
                "######",
                "#P...#",
                "#....#",
                "#....#",
                "######",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 3.5, y = 2.5, text = "Thank you\nfor playing.", height = 0.6,
                  color = {1, 0.95, 0.88} },
            },
            triggers = {},
            is_ending = true,
        },
    }
end

function FPSMode:load_room(index)
    if index > #self.rooms then
        -- Loop or end
        index = 1
    end
    self.room_index = index

    local room = self.rooms[index]
    self.map = FPSMap:new(room)
    self.player:spawn(self.map.spawn)
    self.player.is_dead = false
    self.player.health = self.player.max_health
    self.time = 0
    self.hard_cut_pending = false
    self.hard_cut_timer = 0
    self.screen_text = nil
    self.screen_text_timer = 0
    self.triggered = {}
    self.can_shoot = room.can_shoot == true
    self.linger_timer = 0

    -- Passive enemies don't chase
    if room.passive_enemies then
        for _, sprite in ipairs(self.map.sprites) do
            if sprite.type == "enemy" or sprite.type == "demon" then
                sprite.speed = 0
                sprite.attack_range = 0
            end
        end
    end

    -- Set raycaster palette
    self.raycaster.fog_color = room.fog_color or { 0.02, 0.02, 0.04 }
    self.raycaster.floor_color = room.floor_color or { 0.12, 0.13, 0.16 }
    self.raycaster.ceiling_color = room.ceiling_color or { 0.06, 0.06, 0.08 }

    -- Hard cut visual on room change (except first room)
    if index > 1 then
        self.hard_cut_timer = -0.06  -- brief black frame before scene appears
    end
end

function FPSMode:check_completion()
    local room = self.rooms[self.room_index]
    if not room then return false end

    local completion = room.completion or "exit"

    if completion == "exit" then
        if not self.map.exit then return false end
        local dx = self.player.x - self.map.exit.x
        local dy = self.player.y - self.map.exit.y
        return dx * dx + dy * dy < 0.5
    elseif completion == "kill_all" then
        return not self.map:enemies_alive()
    elseif completion == "linger" then
        return self.time >= (room.linger or 5)
    elseif completion == "trigger" then
        return self.triggered[room.completion_trigger or ""] == true
    end

    return false
end

function FPSMode:trigger_hard_cut()
    if self.hard_cut_pending then return end
    self.hard_cut_pending = true
    self.hard_cut_timer = 0

    local room = self.rooms[self.room_index]
    -- Longer pause on ending
    if room and room.is_ending then
        self.hard_cut_duration = 3
    else
        self.hard_cut_duration = 0.15
    end
end

function FPSMode:update(dt)
    -- Hard cut transition
    if self.hard_cut_pending then
        self.hard_cut_timer = self.hard_cut_timer + dt
        if self.hard_cut_timer > self.hard_cut_duration then
            self.hard_cut_pending = false
            local room = self.rooms[self.room_index]
            if room and room.is_ending then
                -- Restart from beginning
                self:load_room(1)
            else
                self:load_room(self.room_index + 1)
            end
        end
        return
    end

    -- Brief black on room entry
    if self.hard_cut_timer < 0 then
        self.hard_cut_timer = self.hard_cut_timer + dt
        return
    end

    self.time = self.time + dt

    -- Screen text timer
    if self.screen_text_timer > 0 then
        self.screen_text_timer = self.screen_text_timer - dt
        if self.screen_text_timer <= 0 then
            self.screen_text = nil
        end
    end

    if self.player.is_dead then
        if self.time > 1 then
            self.player:spawn(self.map.spawn)
            self.player.is_dead = false
            self.time = 0
        end
        return
    end

    self.player:update(dt, self.map)

    -- Enemy AI (only for non-passive)
    local room = self.rooms[self.room_index]
    for _, sprite in ipairs(self.map.sprites) do
        if sprite.active and (sprite.type == "enemy" or sprite.type == "demon") then
            if sprite.speed > 0 then
                local dx = self.player.x - sprite.x
                local dy = self.player.y - sprite.y
                local dist = math.sqrt(dx * dx + dy * dy)

                if dist < 8 and dist > 0.5 then
                    local nx = dx / dist * sprite.speed * dt
                    local ny = dy / dist * sprite.speed * dt
                    if not self.map:is_solid(sprite.x + nx, sprite.y) then
                        sprite.x = sprite.x + nx
                    end
                    if not self.map:is_solid(sprite.x, sprite.y + ny) then
                        sprite.y = sprite.y + ny
                    end
                end

                sprite.attack_cooldown = math.max(0, (sprite.attack_cooldown or 0) - dt)
                if dist < (sprite.attack_range or 1.5) and sprite.attack_cooldown <= 0 then
                    self.player:take_damage(1, self.sfx)
                    sprite.attack_cooldown = 1.5
                end
            end
        end
    end

    -- Check triggers
    for _, trigger in ipairs(self.map.triggers) do
        if not self.triggered[trigger.id] then
            local delay = trigger.delay or 0
            if self.time >= delay then
                local dx = self.player.x - trigger.x
                local dy = self.player.y - trigger.y
                if dx * dx + dy * dy < trigger.radius * trigger.radius then
                    self.triggered[trigger.id] = true
                    if trigger.screen_text then
                        self.screen_text = trigger.screen_text
                        self.screen_text_timer = trigger.duration or 3
                    end
                end
            end
        end
    end

    -- Check completion
    if self:check_completion() then
        self:trigger_hard_cut()
    end
end

function FPSMode:draw()
    local w = love.graphics.getWidth()
    local h = love.graphics.getHeight()

    love.graphics.setCanvas(self.canvas)
    love.graphics.clear(0, 0, 0, 1)

    -- Hard cut = black screen
    if self.hard_cut_pending or self.hard_cut_timer < 0 then
        love.graphics.setColor(0, 0, 0, 1)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.setCanvas()
        love.graphics.draw(self.canvas, 0, 0, 0, w / RENDER_WIDTH, h / RENDER_HEIGHT)
        return
    end

    if not self.map then
        love.graphics.setCanvas()
        return
    end

    -- Render 3D scene
    local rays = self.raycaster:cast_walls(self.map, self.player.x, self.player.y, self.player.angle)
    self.raycaster:draw_scene(rays)
    self.raycaster:draw_sprites(self.map.sprites, self.player.x, self.player.y, self.player.angle)

    -- World text
    self.raycaster:draw_world_text(self.map.world_texts, self.player.x, self.player.y, self.player.angle)

    -- Exit beacon (subtle glow)
    if self.map.exit then
        local dx = self.map.exit.x - self.player.x
        local dy = self.map.exit.y - self.player.y
        local dist = math.sqrt(dx * dx + dy * dy)
        local exit_angle = math.atan2(dy, dx) - self.player.angle
        while exit_angle > math.pi do exit_angle = exit_angle - math.pi * 2 end
        while exit_angle < -math.pi do exit_angle = exit_angle + math.pi * 2 end

        if math.abs(exit_angle) < math.pi / 3 and dist < 10 then
            local screen_x = (exit_angle / (math.pi / 3) + 0.5) * RENDER_WIDTH
            local pulse = 0.3 + 0.2 * math.sin(self.time * 2)
            love.graphics.setColor(0.9, 0.8, 0.6, pulse / math.max(1, dist * 0.8))
            love.graphics.circle("fill", screen_x, RENDER_HEIGHT / 2, 12 / dist)
        end
    end

    -- Weapon (only if allowed)
    if self.can_shoot then
        self.raycaster:draw_weapon(self.player.weapon_state, self.player.move_time)
        self.raycaster:draw_crosshair()
    end

    -- Screen text (narrative overlay)
    if self.screen_text and self.screen_text_timer > 0 then
        local alpha = 1
        if self.screen_text_timer < 0.5 then
            alpha = self.screen_text_timer / 0.5
        end
        -- Subtle background bar
        love.graphics.setColor(0, 0, 0, 0.4 * alpha)
        love.graphics.rectangle("fill", 0, RENDER_HEIGHT - 50, RENDER_WIDTH, 40)
        -- Text
        love.graphics.setColor(0.88, 0.9, 0.95, alpha)
        love.graphics.printf(
            self.screen_text,
            12, RENDER_HEIGHT - 46, RENDER_WIDTH - 24, "left"
        )
    end

    -- Damage flash
    if self.player.damage_flash > 0 then
        love.graphics.setColor(0.7, 0.08, 0.04, self.player.damage_flash * 0.3)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    -- Death
    if self.player.is_dead then
        love.graphics.setColor(0, 0, 0, 0.7)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.setCanvas()

    -- Draw canvas scaled to window
    love.graphics.draw(self.canvas, 0, 0, 0, w / RENDER_WIDTH, h / RENDER_HEIGHT)
end

function FPSMode:keypressed(key)
    if key == "escape" then
        love.mouse.setRelativeMode(false)
        love.event.quit()
    end
    if key == "tab" then
        self:load_room(self.room_index)
    end
end

function FPSMode:mousemoved(_, _, dx)
    self.player:mousemoved(dx)
end

function FPSMode:mousepressed(_, _, button)
    if button == 1 and self.can_shoot then
        local hit = self.player:fire(self.map, self.sfx)
        -- Check for on_kill callback
        if hit then
            local room = self.rooms[self.room_index]
            if room and room.on_kill then
                room.on_kill(self)
            end
        end
    end
end

return FPSMode
