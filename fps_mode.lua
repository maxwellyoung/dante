-- fps_mode.lua
-- Lo-fi FPS vignette engine.
-- Rooms that DO things. Walls that move. The game breaking itself.

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
    instance.can_shoot = false
    instance.ambient_source = nil
    instance.footstep_timer = 0
    instance.visit_count = 0
    -- Screen effects
    instance.static_intensity = 0
    instance.wobble = 0
    instance.invert_controls = false
    instance.speed_mult = 1
    instance.chase_entity = nil
    instance.room_tint = nil
    instance.flash_timer = 0

    -- Menu state
    instance.in_menu = true
    instance.menu_selection = 1

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
        elseif character == "cold" then
            sample = math.sin(t * tone * math.pi * 2) * 0.3
                   + math.sin(t * (tone * 1.01) * math.pi * 2) * 0.3
        elseif character == "dread" then
            local lfo = math.sin(t * 0.2 * math.pi * 2)
            sample = math.sin(t * tone * (1 + lfo * 0.05) * math.pi * 2) * 0.4
                   + (math.random() * 2 - 1) * 0.04
        elseif character == "silence" then
            sample = (math.random() * 2 - 1) * 0.005
        elseif character == "chase" then
            local urgency = math.sin(t * 2 * math.pi * 2) * 0.3 + 0.7
            sample = math.sin(t * tone * urgency * math.pi * 2) * 0.5
                   + (math.random() * 2 - 1) * 0.08
        elseif character == "glitch" then
            if math.random() < 0.1 then
                sample = (math.random() * 2 - 1) * 0.6
            else
                sample = math.sin(t * tone * math.pi * 2) * 0.2
            end
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

        -- 1. CORRIDOR. Walk forward. On replay, it's longer and darker.
        {
            ambient_tone = 55, ambient_volume = 0.04, ambient_character = "warm",
            fog_color = { 0.02, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = false,
            grid = {
                "################",
                "#P............X#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            on_update = function(self)
                if self.visit_count > 0 and not self.triggered["replay_fog"] then
                    self.triggered["replay_fog"] = true
                    -- Darker on each replay
                    local darkness = math.min(self.visit_count * 0.3, 0.8)
                    self.raycaster.fog_color = {
                        0.02 * (1 - darkness),
                        0.02 * (1 - darkness),
                        0.04 * (1 - darkness)
                    }
                    self.raycaster.max_depth = math.max(4, 16 - self.visit_count * 4)
                    self.screen_text = "Again."
                    self.screen_text_timer = 2
                end
            end,
        },

        -- 2. THE ROOM SHRINKS. Walls close in over 12 seconds. Get out.
        {
            ambient_tone = 50, ambient_volume = 0.06, ambient_character = "dread",
            fog_color = { 0.03, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = false,
            grid = {
                "################",
                "#..............#",
                "#..............#",
                "#..P...........#",
                "#..............#",
                "#..............#",
                "#.............X#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            on_update = function(self, dt)
                -- Walls close in from left side
                local col = 2 + math.floor(self.time / 1.5)
                if col < 13 then
                    for row = 1, 6 do
                        self.map:fill_tile(col - 1, row)
                    end
                end
            end,
        },

        -- 3. GUN. Something at the end. Shoot it. Hard cut the instant it dies.
        -- Enemy is stationary. You have time.
        {
            ambient_tone = 40, ambient_volume = 0.07, ambient_character = "dread",
            fog_color = { 0.05, 0.02, 0.03 },
            floor_color = { 0.14, 0.08, 0.1 },
            ceiling_color = { 0.06, 0.03, 0.04 },
            completion = "trigger", completion_trigger = "killed",
            can_shoot = true,
            passive_enemies = true,
            grid = {
                "##############",
                "#P..........E#",
                "#............#",
                "#............#",
                "##############",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {
                { x = 3, y = 2, radius = 2, id = "gun_hint",
                  screen_text = "Left click to shoot.", duration = 3 },
            },
            on_kill = function(self)
                self.triggered["killed"] = true
            end,
        },

        -- 4. DARK. Nearly black. Shotgun pickup. Something is breathing.
        {
            ambient_tone = 30, ambient_volume = 0.08, ambient_character = "dread",
            fog_color = { 0.005, 0.005, 0.008 },
            floor_color = { 0.02, 0.02, 0.03 },
            ceiling_color = { 0.01, 0.01, 0.015 },
            completion = "exit", can_shoot = true,
            grid = {
                "##############",
                "#P..S........#",
                "#............#",
                "#............#",
                "#............#",
                "#............#",
                "#...........X#",
                "##############",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {
                { x = 7, y = 3, radius = 3, id = "dark",
                  screen_text = "Something is here.", duration = 2 },
            },
            -- Override fog to be extremely short range
            override_fog = 3,
        },

        -- 5. CHASE. Something behind you. Run. Corridors with turns — decisions under panic.
        {
            ambient_tone = 65, ambient_volume = 0.1, ambient_character = "chase",
            fog_color = { 0.06, 0.02, 0.02 },
            floor_color = { 0.12, 0.06, 0.06 },
            ceiling_color = { 0.05, 0.02, 0.02 },
            completion = "exit", can_shoot = false,
            speed_mult = 1.5,
            grid = {
                "###################",
                "#P................#",
                "####.##########...#",
                "#..............#..#",
                "#..########....#..#",
                "#..#.......#......#",
                "#..#.......########",
                "#..#..............#",
                "#..########.####..#",
                "#................X#",
                "###################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            chase = { start_x = 1.5, start_y = 1.5, speed = 2.2, color = {0.9, 0.15, 0.1} },
        },

        -- 6. YOUR CONTROLS ARE WRONG. Left is right. Forward is back.
        -- Small room, exit is obvious. But your body lies to you.
        {
            ambient_tone = 55, ambient_volume = 0.06, ambient_character = "glitch",
            fog_color = { 0.04, 0.03, 0.05 },
            floor_color = { 0.1, 0.1, 0.14 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = false,
            invert_controls = true,
            static_base = 0.15,
            grid = {
                "##########",
                "#........#",
                "#........#",
                "#...P....#",
                "#........#",
                "#.......X#",
                "##########",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {
                { x = 5, y = 3, radius = 2, id = "invert",
                  screen_text = "Something is wrong.", duration = 2 },
            },
        },

        -- 7. MAZE. But the walls keep changing. Bouncer pickup inside.
        {
            ambient_tone = 52, ambient_volume = 0.06, ambient_character = "cold",
            fog_color = { 0.03, 0.03, 0.05 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = true,
            grid = {
                "################",
                "#P.#.....B.....#",
                "#..#.#####.###.#",
                "#..#.......#...#",
                "#..#####.#.#.###",
                "#........#.....#",
                "#.######.####..#",
                "#..............#",
                "#.####.########",
                "#.............X#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            on_update = function(self, dt)
                -- Every 3 seconds, randomly open or close a wall tile
                if math.floor(self.time / 2) ~= math.floor((self.time - dt) / 2) then
                    local attempts = 3
                    for _ = 1, attempts do
                        local rx = math.random(2, 14)
                        local ry = math.random(2, 9)
                        local tile = self.map:get(rx - 1, ry - 1)
                        -- Don't touch player position or exit
                        local px = math.floor(self.player.x)
                        local py = math.floor(self.player.y)
                        if not (rx - 1 == px and ry - 1 == py) then
                            if tile > 0 then
                                self.map:clear_tile(rx - 1, ry - 1)
                            else
                                self.map:fill_tile(rx - 1, ry - 1)
                            end
                        end
                    end
                end
            end,
        },

        -- 8. STILL. Nothing happens. But the room is slowly getting darker.
        -- At 4 seconds, a pillar appears in the center that wasn't there before.
        {
            ambient_tone = 55, ambient_volume = 0.02, ambient_character = "silence",
            fog_color = { 0.02, 0.02, 0.03 },
            floor_color = { 0.08, 0.08, 0.1 },
            ceiling_color = { 0.03, 0.03, 0.04 },
            completion = "linger", linger = 8,
            can_shoot = false,
            grid = {
                "############",
                "#..........#",
                "#..........#",
                "#..........#",
                "#..........#",
                "#.....P....#",
                "#..........#",
                "#..........#",
                "#..........#",
                "############",
            },
            spawn_angle = 3.14,  -- facing away from center
            world_texts = {},
            triggers = {},
            on_update = function(self)
                -- Darken fog over time
                local t = math.min(1, self.time / 8)
                self.raycaster.fog_start = 6 - t * 4
                self.raycaster.max_depth = 16 - t * 10
                -- At 4s, place a pillar behind the player
                if self.time > 4 and not self.triggered["pillar"] then
                    self.triggered["pillar"] = true
                    self.map:fill_tile(5, 4)
                    self.map:fill_tile(6, 4)
                    self.sfx:play("hit_wall")
                end
            end,
        },

        -- 9. ENEMIES. Real ones. Tight T-shaped room. They're close.
        -- You hear them before you see them. Short and violent.
        {
            ambient_tone = 45, ambient_volume = 0.08, ambient_character = "chase",
            fog_color = { 0.06, 0.02, 0.03 },
            floor_color = { 0.15, 0.08, 0.08 },
            ceiling_color = { 0.06, 0.03, 0.03 },
            completion = "kill_all", can_shoot = true,
            override_fog = 6,
            grid = {
                "##########",
                "#...P....#",
                "#........#",
                "###....###",
                "  #....#  ",
                "  #.E..#  ",
                "  #....#  ",
                "###....###",
                "#.E..E...#",
                "#........#",
                "##########",
            },
            spawn_angle = 3.14,
            world_texts = {},
            triggers = {},
        },

        -- 10. THE SAME CORRIDOR. But backwards. Auto rifle pickup.
        {
            ambient_tone = 55, ambient_volume = 0.04, ambient_character = "warm",
            fog_color = { 0.02, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = true,
            grid = {
                "################",
                "#X.......R....P#",
                "################",
            },
            spawn_angle = 3.14,
            world_texts = {},
            triggers = {
                { x = 8, y = 1.5, radius = 2, id = "deja_vu",
                  screen_text = "You've been here before.", duration = 2 },
            },
        },

        -- 11. THE ROOM THAT ISN'T THERE. Walk through a door.
        -- The room behind you is gone. Just wall.
        {
            ambient_tone = 38, ambient_volume = 0.05, ambient_character = "cold",
            fog_color = { 0.02, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = false,
            grid = {
                "################",
                "#P.....#......X#",
                "#......#.......#",
                "#......#.......#",
                "#..............#",
                "#......#.......#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            on_update = function(self)
                -- When player crosses the midpoint, seal the left half
                if self.player.x > 7 and not self.triggered["sealed"] then
                    self.triggered["sealed"] = true
                    for row = 1, 5 do
                        self.map:fill_tile(6, row)
                    end
                    self.screen_text = "You can't go back."
                    self.screen_text_timer = 2
                    self.sfx:play("hit_wall")
                end
            end,
        },

        -- 11. STATIC. Screen fills with noise. Walk through it.
        -- Resolution of the static clears as you approach the exit.
        {
            ambient_tone = 60, ambient_volume = 0.08, ambient_character = "glitch",
            fog_color = { 0.04, 0.04, 0.06 },
            floor_color = { 0.1, 0.1, 0.14 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = false,
            static_base = 0.5,
            grid = {
                "##################",
                "#P...............X#",
                "##################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            on_update = function(self)
                -- Static clears as you approach the exit
                local progress = math.max(0, (self.player.x - 1) / 16)
                self.static_intensity = (1 - progress) * 0.6
            end,
        },

        -- 12. END. Tiny room. Gets smaller each loop.
        {
            ambient_tone = 55, ambient_volume = 0.015, ambient_character = "warm",
            fog_color = { 0.01, 0.01, 0.02 },
            floor_color = { 0.05, 0.05, 0.07 },
            ceiling_color = { 0.02, 0.02, 0.03 },
            completion = "linger", linger = 3,
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
            world_texts = {},
            triggers = {},
            on_update = function(self)
                -- Each loop, the room gets smaller
                if self.visit_count > 0 then
                    local shrink = math.min(self.visit_count, 2)
                    for i = 1, shrink do
                        for row = 0, 3 do
                            self.map:fill_tile(4 - i, row)
                        end
                    end
                end
            end,
        },
    }
end

function FPSMode:load_room(index)
    if index > #self.rooms then
        index = 1
        self.visit_count = self.visit_count + 1
        -- Reset weapons on loop
        self.player.weapons = { "pistol" }
        self.player.current_weapon = 1
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
    self.static_intensity = room.static_base or 0
    self.invert_controls = room.invert_controls == true
    self.speed_mult = room.speed_mult or 1
    self.chase_entity = nil
    self.room_tint = nil
    self.flash_timer = 0

    -- Speed override
    if self.speed_mult ~= 1 then
        -- Modify player speed directly won't work cleanly, handle in update
    end

    -- Chase entity
    if room.chase then
        self.chase_entity = {
            x = room.chase.start_x,
            y = room.chase.start_y,
            speed = room.chase.speed or 2,
            color = room.chase.color or {1, 0.2, 0.1},
            active = true,
            has_face = true,
            width_ratio = 1.2,
        }
        table.insert(self.map.sprites, self.chase_entity)
    end

    -- Passive enemies
    if room.passive_enemies then
        for _, sprite in ipairs(self.map.sprites) do
            if sprite.type == "enemy" or sprite.type == "demon" then
                sprite.speed = 0
                sprite.attack_range = 0
            end
        end
    end

    -- Reset fog distance every room load
    self.raycaster.max_depth = room.override_fog or 16
    self.raycaster.fog_start = 6

    -- Set palette
    self.raycaster.fog_color = room.fog_color or { 0.02, 0.02, 0.04 }
    self.raycaster.floor_color = room.floor_color or { 0.12, 0.13, 0.16 }
    self.raycaster.ceiling_color = room.ceiling_color or { 0.06, 0.06, 0.08 }

    -- Ambient
    self:generate_ambient(room)

    -- Black frame on transition
    if index > 1 then
        self.hard_cut_timer = -0.08
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
    self.hard_cut_duration = (room and room.is_ending) and 2.5 or 0.12
end

function FPSMode:update(dt)
    if self.in_menu then return end

    -- Hard cut
    if self.hard_cut_pending then
        self.hard_cut_timer = self.hard_cut_timer + dt
        if self.hard_cut_timer > self.hard_cut_duration then
            self.hard_cut_pending = false
            local room = self.rooms[self.room_index]
            if room and room.is_ending then
                self:load_room(1)
            else
                self:load_room(self.room_index + 1)
            end
        end
        return
    end

    if self.hard_cut_timer < 0 then
        self.hard_cut_timer = self.hard_cut_timer + dt
        return
    end

    self.time = self.time + dt
    self.flash_timer = math.max(0, self.flash_timer - dt)

    -- Screen text
    if self.screen_text_timer > 0 then
        self.screen_text_timer = self.screen_text_timer - dt
        if self.screen_text_timer <= 0 then self.screen_text = nil end
    end

    if self.player.is_dead then
        if self.time > 0.8 then
            self:load_room(self.room_index)
        end
        return
    end

    -- Apply speed multiplier
    local saved_speed = self.player.move_time
    self.player:update(dt * self.speed_mult, self.map)

    -- Invert controls: flip the angle delta
    -- (handled in mousemoved)

    -- Footsteps
    local moving = love.keyboard.isDown("w", "a", "s", "d", "up", "down")
    if moving and not self.player.is_dead then
        self.footstep_timer = self.footstep_timer - dt
        if self.footstep_timer <= 0 then
            self.sfx:play("footstep")
            self.footstep_timer = 0.35
        end
    else
        self.footstep_timer = 0
    end

    -- Room-specific update
    local room = self.rooms[self.room_index]
    if room and room.on_update then
        room.on_update(self, dt)
    end

    -- Chase entity
    if self.chase_entity and self.chase_entity.active then
        local dx = self.player.x - self.chase_entity.x
        local dy = self.player.y - self.chase_entity.y
        local dist = math.sqrt(dx * dx + dy * dy)
        if dist > 0.3 then
            local nx = dx / dist * self.chase_entity.speed * dt
            local ny = dy / dist * self.chase_entity.speed * dt
            if not self.map:is_solid(self.chase_entity.x + nx, self.chase_entity.y) then
                self.chase_entity.x = self.chase_entity.x + nx
            end
            if not self.map:is_solid(self.chase_entity.x, self.chase_entity.y + ny) then
                self.chase_entity.y = self.chase_entity.y + ny
            end
        end
        -- Caught = restart room
        if dist < 0.5 then
            self.flash_timer = 0.3
            self.sfx:play("death")
            self:load_room(self.room_index)
            return
        end
    end

    -- Enemy AI
    for _, sprite in ipairs(self.map.sprites) do
        if sprite.active and sprite ~= self.chase_entity and (sprite.type == "enemy" or sprite.type == "demon") then
            if (sprite.speed or 0) > 0 then
                local dx = self.player.x - sprite.x
                local dy = self.player.y - sprite.y
                local dist = math.sqrt(dx * dx + dy * dy)
                if dist < 8 and dist > 0.5 then
                    local nx = dx / dist * sprite.speed * dt
                    local ny = dy / dist * sprite.speed * dt
                    if not self.map:is_solid(sprite.x + nx, sprite.y) then sprite.x = sprite.x + nx end
                    if not self.map:is_solid(sprite.x, sprite.y + ny) then sprite.y = sprite.y + ny end
                end
                sprite.attack_cooldown = math.max(0, (sprite.attack_cooldown or 0) - dt)
                if dist < (sprite.attack_range or 1.5) and sprite.attack_cooldown <= 0 then
                    self.player:take_damage(1, self.sfx)
                    sprite.attack_cooldown = 1.5
                end
            end
        end
    end

    -- Triggers
    for _, trigger in ipairs(self.map.triggers) do
        if not self.triggered[trigger.id] then
            if self.time >= (trigger.delay or 0) then
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

    -- Completion
    if self:check_completion() then
        self:trigger_hard_cut()
    end
end

function FPSMode:draw()
    local w = love.graphics.getWidth()
    local h = love.graphics.getHeight()

    if self.in_menu then
        love.graphics.setCanvas(self.canvas)
        love.graphics.clear(0.02, 0.02, 0.03, 1)

        -- Title
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.printf("INFERNAL ASCENT", 0, RENDER_HEIGHT / 2 - 40, RENDER_WIDTH, "center")

        love.graphics.setColor(0.6, 0.62, 0.7, 0.8)
        love.graphics.printf("A descent in first person", 0, RENDER_HEIGHT / 2 - 20, RENDER_WIDTH, "center")

        -- Controls
        love.graphics.setColor(0.5, 0.52, 0.6, 0.6)
        love.graphics.printf("WASD move    Mouse look    Shift sprint", 0, RENDER_HEIGHT / 2 + 10, RENDER_WIDTH, "center")
        love.graphics.printf("Left click shoot    Space wall-jump    Q switch weapon", 0, RENDER_HEIGHT / 2 + 24, RENDER_WIDTH, "center")

        -- Start prompt
        local pulse = 0.5 + 0.5 * math.sin(love.timer.getTime() * 3)
        love.graphics.setColor(0.92, 0.55, 0.18, pulse)
        love.graphics.printf("PRESS ENTER TO DESCEND", 0, RENDER_HEIGHT / 2 + 50, RENDER_WIDTH, "center")

        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.setCanvas()
        love.graphics.draw(self.canvas, 0, 0, 0, w / RENDER_WIDTH, h / RENDER_HEIGHT)
        return
    end

    love.graphics.setCanvas(self.canvas)
    love.graphics.clear(0, 0, 0, 1)

    -- Hard cut / black frame
    if self.hard_cut_pending or self.hard_cut_timer < 0 then
        love.graphics.setCanvas()
        love.graphics.setColor(0, 0, 0, 1)
        love.graphics.rectangle("fill", 0, 0, w, h)
        love.graphics.setColor(1, 1, 1, 1)
        return
    end

    if not self.map then
        love.graphics.setCanvas()
        return
    end

    -- Render 3D
    local rays = self.raycaster:cast_walls(self.map, self.player.x, self.player.y, self.player:get_view_angle())
    self.raycaster:draw_scene(rays)
    self.raycaster:draw_sprites(self.map.sprites, self.player.x, self.player.y, self.player:get_view_angle())
    self.raycaster:draw_world_text(self.map.world_texts, self.player.x, self.player.y, self.player:get_view_angle())

    -- Exit beacon
    if self.map.exit then
        local dx = self.map.exit.x - self.player.x
        local dy = self.map.exit.y - self.player.y
        local dist = math.sqrt(dx * dx + dy * dy)
        local ea = math.atan2(dy, dx) - self.player:get_view_angle()
        while ea > math.pi do ea = ea - math.pi * 2 end
        while ea < -math.pi do ea = ea + math.pi * 2 end
        if math.abs(ea) < math.pi / 3 and dist < 10 then
            local sx = (ea / (math.pi / 3) + 0.5) * RENDER_WIDTH
            local pulse = 0.3 + 0.2 * math.sin(self.time * 2)
            love.graphics.setColor(0.9, 0.8, 0.6, pulse / math.max(1, dist * 0.8))
            love.graphics.circle("fill", sx, RENDER_HEIGHT / 2, 12 / dist)
        end
    end

    -- Weapon
    if self.can_shoot then
        local weapon, weapon_name = self.player:get_weapon()
        self.raycaster:draw_weapon(
            self.player.weapon_state, self.player.move_time,
            weapon_name, weapon.color
        )
        self.raycaster:draw_crosshair()
        -- Weapon name
        love.graphics.setColor(0.7, 0.72, 0.8, 0.5)
        love.graphics.printf(weapon.name, RENDER_WIDTH - 80, RENDER_HEIGHT - 16, 70, "right")
    end

    -- Sprint bar
    self.raycaster:draw_sprint_bar(self.player.sprint_stamina, self.player.is_sprinting)

    -- Health
    if self.player.health < self.player.max_health then
        love.graphics.setColor(0.9, 0.3, 0.2, 0.9)
        love.graphics.printf(
            string.format("VITAE %d", self.player.health),
            8, RENDER_HEIGHT - 16, 80, "left"
        )
    end

    -- Screen effects
    if self.static_intensity > 0 then
        self.raycaster:draw_static(self.static_intensity, self.time)
    end

    -- Chase vignette (red edges when being chased)
    if self.chase_entity and self.chase_entity.active then
        local dx = self.player.x - self.chase_entity.x
        local dy = self.player.y - self.chase_entity.y
        local dist = math.sqrt(dx * dx + dy * dy)
        local urgency = math.max(0, 1 - dist / 8)
        if urgency > 0 then
            self.raycaster:draw_vignette(urgency, {0.6, 0.05, 0.02})
        end
    end

    -- Screen text
    if self.screen_text and self.screen_text_timer > 0 then
        local alpha = math.min(1, self.screen_text_timer / 0.5)
        love.graphics.setColor(0, 0, 0, 0.4 * alpha)
        love.graphics.rectangle("fill", 0, RENDER_HEIGHT - 44, RENDER_WIDTH, 36)
        love.graphics.setColor(0.88, 0.9, 0.95, alpha)
        love.graphics.printf(self.screen_text, 12, RENDER_HEIGHT - 40, RENDER_WIDTH - 24, "left")
    end

    -- Damage flash
    if self.player.damage_flash > 0 then
        love.graphics.setColor(0.7, 0.08, 0.04, self.player.damage_flash * 0.3)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    -- Catch flash (from chase)
    if self.flash_timer > 0 then
        love.graphics.setColor(1, 0.2, 0.1, self.flash_timer)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    -- Death
    if self.player.is_dead then
        love.graphics.setColor(0, 0, 0, 0.7)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.setCanvas()

    love.graphics.draw(self.canvas, 0, 0, 0, w / RENDER_WIDTH, h / RENDER_HEIGHT)
end

function FPSMode:keypressed(key)
    -- Menu
    if self.in_menu then
        if key == "return" or key == "space" then
            self.in_menu = false
            love.mouse.setRelativeMode(true)
            love.mouse.setGrabbed(true)
            self:load_rooms()
            self:load_room(1)
        end
        if key == "escape" then
            love.event.quit()
        end
        return
    end

    if key == "escape" then
        self.in_menu = true
        love.mouse.setRelativeMode(false)
        love.mouse.setGrabbed(false)
        return
    end
    if key == "tab" then
        self:load_room(self.room_index)
    end
    -- Wall-jump
    if key == "space" then
        self.player:try_wall_jump(self.map, self.sfx)
    end
    -- Weapon switch
    if key == "q" or key == "1" or key == "2" or key == "3" or key == "4" then
        if key == "q" then
            self.player:next_weapon()
        else
            local idx = tonumber(key)
            if idx and idx <= #self.player.weapons then
                self.player.current_weapon = idx
            end
        end
    end
end

function FPSMode:mousemoved(_, _, dx)
    local mult = self.invert_controls and -1 or 1
    self.player:mousemoved(dx * mult)
end

function FPSMode:mousepressed(_, _, button)
    if self.in_menu then return end
    if button == 1 and self.can_shoot then
        self.player.holding_fire = true
        local hit = self.player:fire(self.map, self.sfx)
        if hit then
            local room = self.rooms[self.room_index]
            if room and room.on_kill then
                room.on_kill(self)
            end
        end
    end
end

function FPSMode:mousereleased(_, _, button)
    if button == 1 then
        self.player.holding_fire = false
    end
end

return FPSMode
