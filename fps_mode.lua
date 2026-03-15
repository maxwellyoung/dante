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
    instance.footstep_timer = 0
    instance.visit_count = 0  -- how many times we've looped

    love.mouse.setRelativeMode(true)
    love.mouse.setGrabbed(true)

    return instance
end

function FPSMode:generate_ambient(room)
    if self.ambient_source then
        self.ambient_source:stop()
        self.ambient_source = nil
    end

    local tone = room.ambient_tone or 55
    local volume = room.ambient_volume or 0.06
    local character = room.ambient_character or "warm"

    local sample_rate = 44100
    local duration = 3.0
    local count = math.floor(sample_rate * duration)
    local data = love.sound.newSoundData(count, sample_rate, 16, 1)

    for i = 0, count - 1 do
        local t = i / sample_rate
        local env = 1
        local fade = 0.15 * duration
        if t < fade then env = t / fade
        elseif t > duration - fade then env = (duration - t) / fade end

        local sample = 0
        if character == "warm" then
            sample = math.sin(t * tone * math.pi * 2) * 0.4
                   + math.sin(t * (tone + 0.3) * math.pi * 2) * 0.3
                   + math.sin(t * tone * 1.5 * math.pi * 2) * 0.1
        elseif character == "cold" then
            sample = math.sin(t * tone * math.pi * 2) * 0.3
                   + math.sin(t * (tone * 1.01) * math.pi * 2) * 0.3
                   + (math.random() * 2 - 1) * 0.02
        elseif character == "dread" then
            local lfo = math.sin(t * 0.2 * math.pi * 2)
            sample = math.sin(t * tone * (1 + lfo * 0.05) * math.pi * 2) * 0.4
                   + math.sin(t * (tone * 0.5) * math.pi * 2) * 0.2
                   + (math.random() * 2 - 1) * 0.04
        elseif character == "silence" then
            sample = (math.random() * 2 - 1) * 0.008
        elseif character == "hum" then
            sample = math.sin(t * 60 * math.pi * 2) * 0.15
                   + math.sin(t * 120 * math.pi * 2) * 0.08
        end

        data:setSample(i, math.max(-1, math.min(1, sample * volume * env)))
    end

    local source = love.audio.newSource(data)
    source:setLooping(true)
    source:setVolume(volume)
    source:play()
    self.ambient_source = source
end

function FPSMode:load_rooms()
    self.rooms = {
        -- 1. A corridor. Straight ahead. No ambiguity.
        {
            fog_color = { 0.02, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            ambient_tone = 55, ambient_volume = 0.05, ambient_character = "warm",
            completion = "exit",
            can_shoot = false,
            grid = {
                "################",
                "#P............X#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 8.5, y = 1.5, text = "You are descending.", height = 0.5,
                  color = {0.7, 0.72, 0.8} },
            },
            triggers = {},
        },

        -- 2. A room that's too big. Pillar in the center.
        {
            fog_color = { 0.03, 0.02, 0.05 },
            floor_color = { 0.08, 0.08, 0.11 },
            ceiling_color = { 0.03, 0.03, 0.05 },
            ambient_tone = 48, ambient_volume = 0.04, ambient_character = "cold",
            completion = "exit",
            can_shoot = false,
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
                { x = 10, y = 5, radius = 3, id = "room2",
                  screen_text = "The room is bigger than it needs to be.\nThat's the point.", duration = 3.5 },
            },
        },

        -- 3. A turn. Something red at the end. First gun.
        {
            fog_color = { 0.05, 0.02, 0.03 },
            floor_color = { 0.14, 0.08, 0.1 },
            ceiling_color = { 0.06, 0.03, 0.04 },
            ambient_tone = 40, ambient_volume = 0.06, ambient_character = "dread",
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
                { x = 5, y = 2, radius = 1.5, id = "gun",
                  screen_text = "Left click.", duration = 2 },
            },
            on_kill = function(self)
                self.triggered["shot_it"] = true
                self.screen_text = "You did that."
                self.screen_text_timer = 2.5
            end,
        },

        -- 4. An L-shaped room. You hear something around the corner.
        -- There's nothing there.
        {
            fog_color = { 0.03, 0.03, 0.05 },
            floor_color = { 0.11, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            ambient_tone = 52, ambient_volume = 0.07, ambient_character = "dread",
            completion = "exit",
            can_shoot = true,
            grid = {
                "#########",
                "#P......#",
                "#.......#",
                "#.......#",
                "####....#",
                "   #....#",
                "   #....#",
                "   #...X#",
                "   ######",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {
                { x = 5, y = 2, radius = 2, id = "corner_1",
                  screen_text = "Did you hear that?", duration = 2 },
                { x = 5, y = 6, radius = 2, id = "corner_2",
                  screen_text = "There's nothing here.", duration = 2.5 },
            },
        },

        -- 5. No exit. Wait.
        {
            fog_color = { 0.02, 0.02, 0.03 },
            floor_color = { 0.12, 0.12, 0.15 },
            ceiling_color = { 0.05, 0.05, 0.07 },
            ambient_tone = 35, ambient_volume = 0.03, ambient_character = "silence",
            completion = "linger",
            linger = 7,
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
                { x = 4, y = 3, radius = 2, id = "wait_1",
                  screen_text = "There is no exit.", duration = 2.5 },
                { x = 4, y = 3, radius = 2, id = "wait_2", delay = 4,
                  screen_text = "Not yet.", duration = 1.5 },
                { x = 4, y = 3, radius = 2, id = "wait_3", delay = 6,
                  screen_text = "Okay.", duration = 1 },
            },
        },

        -- 6. A maze. But the walls have writing.
        {
            fog_color = { 0.04, 0.03, 0.06 },
            floor_color = { 0.1, 0.1, 0.14 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            ambient_tone = 60, ambient_volume = 0.05, ambient_character = "hum",
            completion = "exit",
            can_shoot = false,
            grid = {
                "################",
                "#P.....#.......#",
                "#.###..#.#####.#",
                "#...#..#.......#",
                "###.#..#.#.###.#",
                "#...#....#...#.#",
                "#.#####.##.#.#.#",
                "#.......#..#...#",
                "#.###.###.####.#",
                "#...........X..#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 4.5, y = 1.5, text = "I keep starting over", height = 0.5,
                  color = {0.5, 0.5, 0.55} },
                { x = 8.5, y = 3.5, text = "each version worse", height = 0.4,
                  color = {0.45, 0.45, 0.5} },
                { x = 2.5, y = 5.5, text = "but I can't stop", height = 0.5,
                  color = {0.55, 0.52, 0.5} },
                { x = 12.5, y = 7.5, text = "because stopping", height = 0.4,
                  color = {0.5, 0.48, 0.48} },
                { x = 6.5, y = 9.5, text = "would mean", height = 0.5,
                  color = {0.6, 0.55, 0.52} },
            },
            triggers = {
                { x = 8, y = 5, radius = 3, id = "maze",
                  screen_text = "You are looking for an exit.\nThe exit is looking for you.", duration = 3.5 },
            },
        },

        -- 7. Long hallway. Text on the walls.
        {
            fog_color = { 0.04, 0.03, 0.06 },
            floor_color = { 0.1, 0.1, 0.14 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            ambient_tone = 45, ambient_volume = 0.04, ambient_character = "cold",
            completion = "exit",
            can_shoot = false,
            grid = {
                "############################",
                "#P........................X#",
                "############################",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 5.5, y = 0.8, text = "every game", height = 0.4, color = {0.45, 0.45, 0.5} },
                { x = 9.5, y = 2.2, text = "is about the person", height = 0.4, color = {0.5, 0.48, 0.5} },
                { x = 14.5, y = 0.8, text = "who made it", height = 0.4, color = {0.55, 0.52, 0.5} },
                { x = 19.5, y = 2.2, text = "pretending", height = 0.4, color = {0.5, 0.48, 0.5} },
                { x = 24.5, y = 1.5, text = "it isn't", height = 0.5, color = {0.7, 0.65, 0.6} },
            },
            triggers = {},
        },

        -- 8. Passive enemies. A choice that doesn't matter.
        {
            fog_color = { 0.06, 0.03, 0.04 },
            floor_color = { 0.15, 0.1, 0.11 },
            ceiling_color = { 0.06, 0.04, 0.05 },
            ambient_tone = 38, ambient_volume = 0.06, ambient_character = "dread",
            completion = "exit",
            can_shoot = true,
            passive_enemies = true,
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
            world_texts = {
                { x = 7.5, y = 1.2, text = "They won't hurt you.", height = 0.5,
                  color = {0.7, 0.72, 0.8} },
            },
            triggers = {
                { x = 7, y = 4, radius = 3, id = "passive",
                  screen_text = "You can shoot them if you want.\nNothing changes.", duration = 3.5 },
            },
        },

        -- 9. A room with two doors. Both lead to the same place.
        {
            fog_color = { 0.03, 0.03, 0.05 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            ambient_tone = 55, ambient_volume = 0.04, ambient_character = "warm",
            completion = "exit",
            can_shoot = false,
            grid = {
                "###############",
                "#P............#",
                "#.....##......#",
                "#.....##......#",
                "#.............#",
                "#.....##......#",
                "#.....##......#",
                "#............X#",
                "###############",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 4.5, y = 2.5, text = "left", height = 0.5, color = {0.6, 0.62, 0.7} },
                { x = 10.5, y = 2.5, text = "right", height = 0.5, color = {0.6, 0.62, 0.7} },
            },
            triggers = {
                { x = 7, y = 4, radius = 2, id = "choice",
                  screen_text = "It doesn't matter which way you go.\nYou already know that.", duration = 3 },
            },
        },

        -- 10. Darkness. Almost nothing visible. One word.
        {
            fog_color = { 0.01, 0.01, 0.01 },
            floor_color = { 0.03, 0.03, 0.04 },
            ceiling_color = { 0.01, 0.01, 0.02 },
            ambient_tone = 30, ambient_volume = 0.03, ambient_character = "silence",
            completion = "linger",
            linger = 6,
            can_shoot = false,
            grid = {
                "########",
                "#P.....#",
                "#......#",
                "#......#",
                "#......#",
                "########",
            },
            spawn_angle = 0,
            world_texts = {
                { x = 4.5, y = 3, text = "Here.", height = 0.5,
                  color = {0.95, 0.92, 0.85} },
            },
            triggers = {
                { x = 4, y = 3, radius = 2, id = "here", delay = 3,
                  screen_text = "This is where it was going the whole time.", duration = 3 },
            },
        },

        -- 11. The end. Thank you.
        {
            fog_color = { 0.01, 0.01, 0.02 },
            floor_color = { 0.05, 0.05, 0.07 },
            ceiling_color = { 0.02, 0.02, 0.03 },
            ambient_tone = 55, ambient_volume = 0.02, ambient_character = "warm",
            completion = "linger",
            linger = 5,
            can_shoot = false,
            is_ending = true,
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
        },
    }
end

function FPSMode:load_room(index)
    if index > #self.rooms then
        index = 1
        self.visit_count = self.visit_count + 1
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

    -- Ambient drone
    self:generate_ambient(room)

    -- Hard cut visual on room change (except first room)
    if index > 1 then
        self.hard_cut_timer = -0.08  -- brief black frame before scene appears
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

    -- Footsteps
    local moving = love.keyboard.isDown("w", "a", "s", "d", "up", "down")
    if moving and not self.player.is_dead then
        self.footstep_timer = self.footstep_timer - dt
        if self.footstep_timer <= 0 then
            self.sfx:play("footstep")
            self.footstep_timer = 0.4
        end
    else
        self.footstep_timer = 0
    end

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
