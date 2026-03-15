-- fps_mode.lua
-- Lo-fi FPS. Rooms that do things. Vlambeer feel. Blendo cuts.

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
    instance.static_intensity = 0
    instance.invert_controls = false
    instance.speed_mult = 1
    instance.chase_entity = nil
    instance.flash_timer = 0
    instance.hitstop_timer = 0
    instance.in_menu = true

    return instance
end

function FPSMode:start_ambient(room)
    if self.ambient_source then
        self.ambient_source:stop()
        self.ambient_source = nil
    end

    local tone = room.ambient_tone or 55
    local vol = room.ambient_volume or 0.015

    local sr = 44100
    local dur = 3.0
    local count = math.floor(sr * dur)
    local data = love.sound.newSoundData(count, sr, 16, 1)

    for i = 0, count - 1 do
        local t = i / sr
        local env = 1
        local fade = 0.2 * dur
        if t < fade then env = t / fade
        elseif t > dur - fade then env = (dur - t) / fade end

        local s = math.sin(t * tone * math.pi * 2) * 0.35
                + math.sin(t * (tone + 0.4) * math.pi * 2) * 0.25
                + math.sin(t * tone * 1.5 * math.pi * 2) * 0.08
        data:setSample(i, math.max(-1, math.min(1, s * vol * env)))
    end

    local source = love.audio.newSource(data)
    source:setLooping(true)
    source:setVolume(vol)
    source:play()
    self.ambient_source = source
end

function FPSMode:load_rooms()
    self.rooms = {

        -- 1. CORRIDOR + FIRST GUN. Walk, pick up gun, shoot one thing. Done.
        {
            ambient_tone = 55, ambient_volume = 0.012,
            fog_color = { 0.02, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.13 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "trigger", completion_trigger = "killed",
            can_shoot = true, passive_enemies = true,
            grid = {
                "####################",
                "#P..S..............E#",
                "#..................#",
                "####################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {
                { x = 4, y = 1.5, radius = 2, id = "hint",
                  screen_text = "Pick up the gun. Left click to shoot.", duration = 3 },
            },
            on_kill = function(self)
                self.triggered["killed"] = true
            end,
        },

        -- 2. L-SHAPED COMBAT. Enemies around a corner. Shotgun pickup.
        {
            ambient_tone = 48, ambient_volume = 0.015,
            fog_color = { 0.04, 0.02, 0.04 },
            floor_color = { 0.12, 0.1, 0.12 },
            ceiling_color = { 0.05, 0.04, 0.05 },
            completion = "kill_all", can_shoot = true,
            grid = {
                "##############",
                "#P...........#",
                "#............#",
                "#............#",
                "#####........#",
                "    #........#",
                "    #...E....#",
                "    #........#",
                "    #....E...#",
                "    #........#",
                "    ##########",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {
                { x = 5, y = 6, radius = 3, id = "corner",
                  screen_text = "Around the corner.", duration = 2 },
            },
        },

        -- 3. CHASE. L-shaped escape route. Something behind you.
        {
            ambient_tone = 65, ambient_volume = 0.018,
            fog_color = { 0.06, 0.02, 0.02 },
            floor_color = { 0.12, 0.06, 0.06 },
            ceiling_color = { 0.05, 0.02, 0.02 },
            completion = "exit", can_shoot = false,
            speed_mult = 1.5,
            grid = {
                "################",
                "#P.............#",
                "#.......########",
                "#..............#",
                "########.......#",
                "#..............#",
                "#.......########",
                "#.............X#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            chase = { start_x = 1.5, start_y = 1.5, speed = 2.8, color = {0.9, 0.15, 0.1} },
        },

        -- 4. ARENA. Real fight. Demon + walkers. Cover pillars.
        -- Bouncer pickup in the room.
        {
            ambient_tone = 50, ambient_volume = 0.015,
            fog_color = { 0.05, 0.02, 0.03 },
            floor_color = { 0.14, 0.08, 0.1 },
            ceiling_color = { 0.06, 0.03, 0.04 },
            completion = "kill_all", can_shoot = true,
            grid = {
                "################",
                "#P.............#",
                "#..............#",
                "#....##........#",
                "#....##...E....#",
                "#..............#",
                "#.B........##..#",
                "#..........##..#",
                "#..............#",
                "#.....D........#",
                "#..............#",
                "#.........E....#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
        },

        -- 5. THE ROOM SHRINKS. Walls close in. Sprint to the exit.
        {
            ambient_tone = 50, ambient_volume = 0.015,
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
            triggers = {
                { x = 4, y = 3, radius = 2, id = "shrink",
                  screen_text = "The walls are closing. Sprint.", duration = 2 },
            },
            on_update = function(self, dt)
                local col = 2 + math.floor(self.time / 1.2)
                if col < 13 then
                    for row = 1, 6 do
                        self.map:fill_tile(col - 1, row)
                    end
                end
            end,
        },

        -- 6. INVERTED CONTROLS. Simple room. Mouse is backwards.
        {
            ambient_tone = 55, ambient_volume = 0.01,
            fog_color = { 0.04, 0.03, 0.05 },
            floor_color = { 0.1, 0.1, 0.14 },
            ceiling_color = { 0.04, 0.04, 0.06 },
            completion = "exit", can_shoot = false,
            invert_controls = true,
            static_base = 0.1,
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
                  screen_text = "Something is wrong with the controls.", duration = 2.5 },
            },
        },

        -- 7. GAUNTLET. Long corridor with enemies. Auto rifle pickup.
        -- Sprint and shoot. Pure kinetic flow.
        {
            ambient_tone = 55, ambient_volume = 0.015,
            fog_color = { 0.05, 0.02, 0.03 },
            floor_color = { 0.14, 0.08, 0.09 },
            ceiling_color = { 0.06, 0.03, 0.03 },
            completion = "exit", can_shoot = true,
            grid = {
                "##############################",
                "#P..R.....E.....E.....E.....X#",
                "#............................#",
                "##############################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
        },

        -- 8. THE ROOM SEALS. Walk forward. Behind you closes.
        -- Then enemies spawn ahead.
        {
            ambient_tone = 42, ambient_volume = 0.012,
            fog_color = { 0.03, 0.02, 0.04 },
            floor_color = { 0.1, 0.1, 0.12 },
            ceiling_color = { 0.04, 0.04, 0.05 },
            completion = "kill_all", can_shoot = true,
            grid = {
                "################",
                "#P.............#",
                "#..............#",
                "#..............#",
                "#..............#",
                "#..............#",
                "################",
            },
            spawn_angle = 0,
            world_texts = {},
            triggers = {},
            on_update = function(self)
                if self.player.x > 6 and not self.triggered["sealed"] then
                    self.triggered["sealed"] = true
                    -- Seal behind
                    for row = 1, 5 do
                        self.map:fill_tile(5, row)
                    end
                    -- Spawn enemies ahead
                    table.insert(self.map.sprites, {
                        x = 12.5, y = 2.5, type = "enemy",
                        color = {0.85, 0.25, 0.2}, health = 2, active = true,
                        has_face = true, width_ratio = 0.7, speed = 1.4,
                        attack_range = 1.5, attack_cooldown = 0,
                    })
                    table.insert(self.map.sprites, {
                        x = 13.5, y = 4.5, type = "enemy",
                        color = {0.85, 0.25, 0.2}, health = 2, active = true,
                        has_face = true, width_ratio = 0.7, speed = 1.4,
                        attack_range = 1.5, attack_cooldown = 0,
                    })
                    self.screen_text = "You can't go back."
                    self.screen_text_timer = 2
                    self.sfx:play("hit_wall")
                    self.raycaster:shake(4, 0.15)
                end
            end,
        },

        -- 9. BOSS. Big demon in a circular room. Pillars for cover.
        -- It strafes, shoots projectiles, lunges. This is the test.
        {
            ambient_tone = 38, ambient_volume = 0.018,
            fog_color = { 0.06, 0.02, 0.02 },
            floor_color = { 0.16, 0.06, 0.06 },
            ceiling_color = { 0.06, 0.02, 0.02 },
            completion = "kill_all", can_shoot = true,
            grid = {
                "################",
                "#..............#",
                "#..##......##..#",
                "#..............#",
                "#......D.......#",
                "#..............#",
                "#..##......##..#",
                "#.......P......#",
                "################",
            },
            spawn_angle = -1.57,
            world_texts = {},
            triggers = {},
            -- Make demon beefy
            on_load = function(self)
                for _, s in ipairs(self.map.sprites) do
                    if s.type == "demon" then
                        s.health = 8
                        s.speed = 1.2
                    end
                end
            end,
        },

        -- 10. END. Tiny room. Loop.
        {
            ambient_tone = 55, ambient_volume = 0.008,
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
            world_texts = {
                { x = 3.5, y = 2.5, text = "Again.", height = 0.5,
                  color = {0.7, 0.68, 0.65} },
            },
            triggers = {},
        },
    }
end

function FPSMode:load_room(index)
    if index > #self.rooms then
        index = 1
        self.visit_count = self.visit_count + 1
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
    self.flash_timer = 0
    self.hitstop_timer = 0

    -- Chase entity
    if room.chase then
        self.chase_entity = {
            x = room.chase.start_x, y = room.chase.start_y,
            speed = room.chase.speed or 2,
            color = room.chase.color or {1, 0.2, 0.1},
            active = true, has_face = true, width_ratio = 1.2,
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

    -- Fog
    self.raycaster.max_depth = room.override_fog or 16
    self.raycaster.fog_start = 6
    self.raycaster.fog_color = room.fog_color or { 0.02, 0.02, 0.04 }
    self.raycaster.floor_color = room.floor_color or { 0.12, 0.13, 0.16 }
    self.raycaster.ceiling_color = room.ceiling_color or { 0.06, 0.06, 0.08 }

    -- Ambient
    self:start_ambient(room)

    -- Room-specific init
    if room.on_load then room.on_load(self) end

    -- Black frame on transition
    if index > 1 then self.hard_cut_timer = -0.08 end
end

function FPSMode:check_completion()
    local room = self.rooms[self.room_index]
    if not room then return false end
    local c = room.completion or "exit"
    if c == "exit" then
        if not self.map.exit then return false end
        local dx = self.player.x - self.map.exit.x
        local dy = self.player.y - self.map.exit.y
        return dx * dx + dy * dy < 0.5
    elseif c == "kill_all" then
        return not self.map:enemies_alive()
    elseif c == "linger" then
        return self.time >= (room.linger or 5)
    elseif c == "trigger" then
        return self.triggered[room.completion_trigger or ""] == true
    end
    return false
end

function FPSMode:trigger_hard_cut()
    if self.hard_cut_pending then return end
    self.hard_cut_pending = true
    self.hard_cut_timer = 0
    local room = self.rooms[self.room_index]
    self.hard_cut_duration = (room and room.is_ending) and 2 or 0.12
end

function FPSMode:update(dt)
    if self.in_menu then return end

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

    if self.hitstop_timer > 0 then
        self.hitstop_timer = self.hitstop_timer - dt
        self.raycaster:update_shake(dt)
        return
    end

    self.time = self.time + dt
    self.flash_timer = math.max(0, self.flash_timer - dt)
    self.raycaster:update_shake(dt)

    if self.screen_text_timer > 0 then
        self.screen_text_timer = self.screen_text_timer - dt
        if self.screen_text_timer <= 0 then self.screen_text = nil end
    end

    if self.player.is_dead then
        if self.time > 0.8 then self:load_room(self.room_index) end
        return
    end

    self.player:update(dt * self.speed_mult, self.map)

    -- Footsteps
    local moving = love.keyboard.isDown("w", "a", "s", "d", "up", "down")
    if moving and not self.player.is_dead then
        self.footstep_timer = self.footstep_timer - dt
        if self.footstep_timer <= 0 then
            self.sfx:play("footstep")
            self.footstep_timer = self.player.is_sprinting and 0.25 or 0.4
        end
    else
        self.footstep_timer = 0
    end

    -- Room logic
    local room = self.rooms[self.room_index]
    if room and room.on_update then room.on_update(self, dt) end

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

                if not sprite.ai_state then
                    sprite.ai_state = "chase"
                    sprite.strafe_dir = math.random() > 0.5 and 1 or -1
                    sprite.strafe_timer = 0
                    sprite.flinch_timer = 0
                    sprite.fire_cooldown = math.random() * 2
                end

                sprite.attack_cooldown = math.max(0, (sprite.attack_cooldown or 0) - dt)
                sprite.strafe_timer = math.max(0, (sprite.strafe_timer or 0) - dt)
                sprite.flinch_timer = math.max(0, (sprite.flinch_timer or 0) - dt)

                if sprite.flinch_timer > 0 then
                    -- Stunned
                elseif dist < 10 and dist > 0.4 then
                    local speed = sprite.speed
                    local nx, ny = dx / dist, dy / dist

                    if sprite.type == "demon" then
                        if dist > 3 and dist < 7 then
                            if (sprite.strafe_timer or 0) <= 0 then
                                sprite.strafe_dir = -(sprite.strafe_dir or 1)
                                sprite.strafe_timer = 0.8 + math.random() * 0.6
                            end
                            local sx = -ny * (sprite.strafe_dir or 1) * speed * 0.7 * dt + nx * speed * 0.3 * dt
                            local sy = nx * (sprite.strafe_dir or 1) * speed * 0.7 * dt + ny * speed * 0.3 * dt
                            if not self.map:is_solid(sprite.x + sx, sprite.y) then sprite.x = sprite.x + sx end
                            if not self.map:is_solid(sprite.x, sprite.y + sy) then sprite.y = sprite.y + sy end
                        elseif dist <= 3 then
                            local lx = nx * speed * 2.5 * dt
                            local ly = ny * speed * 2.5 * dt
                            if not self.map:is_solid(sprite.x + lx, sprite.y) then sprite.x = sprite.x + lx end
                            if not self.map:is_solid(sprite.x, sprite.y + ly) then sprite.y = sprite.y + ly end
                        else
                            local mx = nx * speed * dt
                            local my = ny * speed * dt
                            if not self.map:is_solid(sprite.x + mx, sprite.y) then sprite.x = sprite.x + mx end
                            if not self.map:is_solid(sprite.x, sprite.y + my) then sprite.y = sprite.y + my end
                        end

                        sprite.fire_cooldown = (sprite.fire_cooldown or 0) - dt
                        if (sprite.fire_cooldown or 0) <= 0 and dist > 2 and dist < 8 then
                            sprite.fire_cooldown = 2.0 + math.random() * 1.5
                            table.insert(self.map.sprites, {
                                x = sprite.x, y = sprite.y,
                                type = "projectile",
                                vx = nx * 4, vy = ny * 4,
                                lifetime = 3, active = true,
                                color = { 1, 0.3, 0.15 },
                                has_face = false, width_ratio = 0.3, damage = 1,
                            })
                            self.sfx:play("shoot")
                        end
                    else
                        local weave = math.sin(self.time * 3 + sprite.x * 7) * 0.3
                        local mx = (nx + weave * (-ny)) * speed * dt
                        local my = (ny + weave * nx) * speed * dt
                        if not self.map:is_solid(sprite.x + mx, sprite.y) then sprite.x = sprite.x + mx end
                        if not self.map:is_solid(sprite.x, sprite.y + my) then sprite.y = sprite.y + my end
                    end
                end

                if dist < (sprite.attack_range or 1.5) and (sprite.attack_cooldown or 0) <= 0 and (sprite.flinch_timer or 0) <= 0 then
                    self.player:take_damage(1, self.sfx)
                    sprite.attack_cooldown = sprite.type == "demon" and 1.0 or 1.5
                    self.raycaster:shake(5, 0.15)
                    self.flash_timer = 0.2
                    self.hitstop_timer = 0.04
                end
            end
        end
    end

    -- Enemy projectiles
    for i = #self.map.sprites, 1, -1 do
        local s = self.map.sprites[i]
        if s.type == "projectile" and s.active then
            s.x = s.x + s.vx * dt
            s.y = s.y + s.vy * dt
            s.lifetime = s.lifetime - dt
            if self.map:is_solid(s.x, s.y) then s.active = false end
            local pdx = self.player.x - s.x
            local pdy = self.player.y - s.y
            if pdx * pdx + pdy * pdy < 0.3 then
                self.player:take_damage(s.damage or 1, self.sfx)
                s.active = false
                self.flash_timer = 0.2
                self.raycaster:shake(6, 0.15)
                self.hitstop_timer = 0.05
            end
            if s.lifetime <= 0 then s.active = false end
        end
    end

    -- Triggers
    for _, trigger in ipairs(self.map.triggers) do
        if not self.triggered[trigger.id] then
            if self.time >= (trigger.delay or 0) then
                local tdx = self.player.x - trigger.x
                local tdy = self.player.y - trigger.y
                if tdx * tdx + tdy * tdy < trigger.radius * trigger.radius then
                    self.triggered[trigger.id] = true
                    if trigger.screen_text then
                        self.screen_text = trigger.screen_text
                        self.screen_text_timer = trigger.duration or 3
                    end
                end
            end
        end
    end

    if self:check_completion() then self:trigger_hard_cut() end
end

function FPSMode:draw()
    local w = love.graphics.getWidth()
    local h = love.graphics.getHeight()

    if self.in_menu then
        love.graphics.setCanvas(self.canvas)
        love.graphics.clear(0.02, 0.02, 0.03, 1)
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.printf("INFERNAL ASCENT", 0, RENDER_HEIGHT / 2 - 40, RENDER_WIDTH, "center")
        love.graphics.setColor(0.5, 0.52, 0.6, 0.6)
        love.graphics.printf("WASD  Mouse  Shift sprint  Space wall-jump", 0, RENDER_HEIGHT / 2 + 4, RENDER_WIDTH, "center")
        love.graphics.printf("Left click shoot  Q switch weapon", 0, RENDER_HEIGHT / 2 + 18, RENDER_WIDTH, "center")
        local pulse = 0.5 + 0.5 * math.sin(love.timer.getTime() * 3)
        love.graphics.setColor(0.92, 0.55, 0.18, pulse)
        love.graphics.printf("PRESS ENTER", 0, RENDER_HEIGHT / 2 + 44, RENDER_WIDTH, "center")
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.setCanvas()
        love.graphics.draw(self.canvas, 0, 0, 0, w / RENDER_WIDTH, h / RENDER_HEIGHT)
        return
    end

    love.graphics.setCanvas(self.canvas)
    love.graphics.clear(0, 0, 0, 1)

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

    local rays = self.raycaster:cast_walls(self.map, self.player.x, self.player.y, self.player:get_view_angle())
    self.raycaster:draw_scene(rays)
    self.raycaster:draw_sprites(self.map.sprites, self.player.x, self.player.y, self.player:get_view_angle())

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
            local p = 0.3 + 0.2 * math.sin(self.time * 2)
            love.graphics.setColor(0.9, 0.8, 0.6, p / math.max(1, dist * 0.8))
            love.graphics.circle("fill", sx, RENDER_HEIGHT / 2, 12 / dist)
        end
    end

    -- Weapon
    if self.can_shoot then
        local weapon, weapon_name = self.player:get_weapon()
        self.raycaster:draw_weapon(self.player.weapon_state, self.player.move_time, weapon_name, weapon.color)
        self.raycaster:draw_crosshair()
        love.graphics.setColor(0.6, 0.62, 0.7, 0.4)
        love.graphics.printf(weapon.name, RENDER_WIDTH - 80, RENDER_HEIGHT - 16, 70, "right")
    end

    self.raycaster:draw_sprint_bar(self.player.sprint_stamina, self.player.is_sprinting)

    -- Health
    love.graphics.setColor(0.9, 0.3, 0.2, 0.9)
    love.graphics.printf(string.format("%d", self.player.health), 8, RENDER_HEIGHT - 16, 40, "left")

    -- Chase vignette
    if self.chase_entity and self.chase_entity.active then
        local cdx = self.player.x - self.chase_entity.x
        local cdy = self.player.y - self.chase_entity.y
        local cdist = math.sqrt(cdx * cdx + cdy * cdy)
        local urgency = math.max(0, 1 - cdist / 8)
        if urgency > 0 then
            self.raycaster:draw_vignette(urgency, {0.6, 0.05, 0.02})
        end
    end

    -- Static
    if self.static_intensity > 0 then
        self.raycaster:draw_static(self.static_intensity, self.time)
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
        love.graphics.setColor(0.7, 0.08, 0.04, self.player.damage_flash * 0.25)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end
    if self.flash_timer > 0 then
        love.graphics.setColor(1, 0.2, 0.1, self.flash_timer * 0.5)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    if self.player.is_dead then
        love.graphics.setColor(0, 0, 0, 0.7)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.setCanvas()

    local shx = self.raycaster.shake_x * (w / RENDER_WIDTH)
    local shy = self.raycaster.shake_y * (h / RENDER_HEIGHT)
    love.graphics.draw(self.canvas, shx, shy, 0, w / RENDER_WIDTH, h / RENDER_HEIGHT)
end

function FPSMode:keypressed(key)
    if self.in_menu then
        if key == "return" or key == "space" then
            self.in_menu = false
            love.mouse.setRelativeMode(true)
            love.mouse.setGrabbed(true)
            self:load_rooms()
            self:load_room(1)
        end
        if key == "escape" then love.event.quit() end
        return
    end
    if key == "escape" then
        self.in_menu = true
        love.mouse.setRelativeMode(false)
        love.mouse.setGrabbed(false)
        if self.ambient_source then self.ambient_source:stop() end
        return
    end
    if key == "tab" then self:load_room(self.room_index) end
    if key == "space" then self.player:try_wall_jump(self.map, self.sfx) end
    if key == "q" then self.player:next_weapon() end
    if key == "1" or key == "2" or key == "3" or key == "4" then
        local idx = tonumber(key)
        if idx and idx <= #self.player.weapons then self.player.current_weapon = idx end
    end
end

function FPSMode:mousemoved(_, _, dx)
    if self.in_menu then return end
    local mult = self.invert_controls and -1 or 1
    self.player:mousemoved(dx * mult)
end

function FPSMode:mousepressed(_, _, button)
    if self.in_menu then return end
    if button == 1 and self.can_shoot then
        self.player.holding_fire = true
        local kills_before = self.player.kills
        local hit = self.player:fire(self.map, self.sfx)
        local weapon = self.player:get_weapon()
        self.raycaster:shake(weapon.recoil * 0.8, 0.08)
        if hit then
            self.raycaster:shake(weapon.recoil * 1.5, 0.12)
            if self.player.kills > kills_before then
                self.hitstop_timer = 0.06
                self.raycaster:shake(4, 0.15)
                self.flash_timer = 0.1
            end
            local room = self.rooms[self.room_index]
            if room and room.on_kill then room.on_kill(self) end
        end
    end
end

function FPSMode:mousereleased(_, _, button)
    if button == 1 then self.player.holding_fire = false end
end

return FPSMode
