-- fps_mode.lua
-- Lo-fi FPS game mode. Raycaster rendering, micro-trial structure.
-- Descend through circles of Hell in first person.

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
    instance.trial_timer = 0
    instance.trial_active = false
    instance.trial_rule = ""
    instance.room_index = 1
    instance.rooms = {}
    instance.hard_cut_timer = 0
    instance.hard_cut_pending = false
    instance.score = 0
    instance.deaths = 0
    instance.trial_score = nil
    instance.circle_transition = nil

    love.mouse.setRelativeMode(true)
    love.mouse.setGrabbed(true)

    return instance
end

function FPSMode:load_rooms()
    self.rooms = {
        -- LIMBO: Find the exit
        {
            title = "LIMBO", subtitle = "Enter.",
            trial_rule = "FIND THE EXIT",
            trial_timer = 20,
            completion = "exit",
            circle_id = "limbo",
            fog_color = { 0.03, 0.03, 0.06 },
            floor_color = { 0.14, 0.15, 0.18 },
            ceiling_color = { 0.06, 0.06, 0.09 },
            grid = {
                "############",
                "#P.........#",
                "#..........#",
                "#...##.....#",
                "#..........#",
                "#......##..#",
                "#..........#",
                "#.....E....#",
                "#..........#",
                "#.........X#",
                "############",
            },
            spawn_angle = 0,
        },
        -- LIMBO: Kill all enemies
        {
            title = "LIMBO", subtitle = "Clear.",
            trial_rule = "KILL EVERYTHING",
            trial_timer = 25,
            completion = "kill_all",
            circle_id = "limbo",
            fog_color = { 0.03, 0.03, 0.06 },
            floor_color = { 0.14, 0.15, 0.18 },
            ceiling_color = { 0.06, 0.06, 0.09 },
            grid = {
                "##############",
                "#P...........#",
                "#............#",
                "#....##......#",
                "#...........E#",
                "#............#",
                "#.E..........#",
                "#......##....#",
                "#............#",
                "#...E........#",
                "#............#",
                "##############",
            },
            spawn_angle = 0,
        },
        -- LIMBO: Navigate maze
        {
            title = "LIMBO", subtitle = "Descend.",
            trial_rule = "FIND THE WAY DOWN",
            trial_timer = 30,
            completion = "exit",
            circle_id = "limbo",
            fog_color = { 0.04, 0.03, 0.06 },
            floor_color = { 0.13, 0.14, 0.17 },
            ceiling_color = { 0.05, 0.05, 0.08 },
            grid = {
                "################",
                "#P.....#.......#",
                "#.###..#.#####.#",
                "#...#..#.#.....#",
                "###.#..#.#.###.#",
                "#...#....#...#.#",
                "#.#####.##.#.#.#",
                "#.......#..#.#.#",
                "#.#####.#.##.#.#",
                "#.#...........E#",
                "#.#.######.####",
                "#..........#..X#",
                "################",
            },
            spawn_angle = 0,
        },

        -- LUST: Tight corridors, more enemies
        {
            title = "LUST", subtitle = "The walls close in.",
            trial_rule = "ESCAPE",
            trial_timer = 25,
            completion = "exit",
            circle_id = "lust",
            fog_color = { 0.08, 0.02, 0.04 },
            floor_color = { 0.18, 0.1, 0.12 },
            ceiling_color = { 0.08, 0.04, 0.06 },
            wall_type = 2,
            grid = {
                "2222222222222",
                "2P..........2",
                "2.2222.2222.2",
                "2..........E2",
                "22222.222.2.2",
                "2.........2.2",
                "2.222.22222.2",
                "2E..........2",
                "2.22222222..2",
                "2..........X2",
                "2222222222222",
            },
            spawn_angle = 0,
        },
        -- LUST: Arena fight
        {
            title = "LUST", subtitle = "Hold the line.",
            trial_rule = "KILL ALL SINNERS",
            trial_timer = 30,
            completion = "kill_all",
            circle_id = "lust",
            fog_color = { 0.08, 0.02, 0.04 },
            floor_color = { 0.18, 0.1, 0.12 },
            ceiling_color = { 0.08, 0.04, 0.06 },
            wall_type = 2,
            grid = {
                "22222222222222",
                "2P...........2",
                "2............2",
                "2....22......2",
                "2.........E..2",
                "2............2",
                "2..E....22...2",
                "2............2",
                "2......E.....2",
                "2....22......2",
                "2..........D.2",
                "2............2",
                "22222222222222",
            },
            spawn_angle = 0,
        },

        -- GLUTTONY: Sludge maze
        {
            title = "GLUTTONY", subtitle = "The mire swallows.",
            trial_rule = "WADE THROUGH",
            trial_timer = 35,
            completion = "exit",
            circle_id = "gluttony",
            fog_color = { 0.04, 0.06, 0.02 },
            floor_color = { 0.14, 0.18, 0.1 },
            ceiling_color = { 0.06, 0.08, 0.04 },
            wall_type = 3,
            grid = {
                "3333333333333",
                "3P..........3",
                "3.333..333..3",
                "3..........E3",
                "3.33.3333.3.3",
                "3..........D3",
                "3.333.333...3",
                "3.......33..3",
                "3E.333......3",
                "3..........X3",
                "3333333333333",
            },
            spawn_angle = 0,
        },
    }
end

function FPSMode:load_room(index)
    if index > #self.rooms then
        index = 1
        self.score = 0
    end
    self.room_index = index

    local room = self.rooms[index]
    self.map = FPSMap:new(room)
    self.player:spawn(self.map.spawn)
    self.time = 0
    self.trial_timer = room.trial_timer or 20
    self.trial_active = self.trial_timer > 0
    self.trial_rule = room.trial_rule or ""
    self.hard_cut_pending = false
    self.hard_cut_timer = 0
    self.trial_score = nil
    self.deaths = 0

    -- Set raycaster palette
    self.raycaster.fog_color = room.fog_color or { 0.02, 0.02, 0.04 }
    self.raycaster.floor_color = room.floor_color or { 0.12, 0.13, 0.16 }
    self.raycaster.ceiling_color = room.ceiling_color or { 0.06, 0.06, 0.08 }

    -- Check circle transition
    if index > 1 then
        local prev_circle = self.rooms[index - 1].circle_id
        local curr_circle = room.circle_id
        if prev_circle ~= curr_circle then
            local messages = {
                lust = { title = "DEEPER", subtitle = "The air thickens with want.", accent = {0.95, 0.46, 0.56} },
                gluttony = { title = "DEEPER STILL", subtitle = "The floor remembers your weight.", accent = {0.4, 0.72, 0.3} },
            }
            local msg = messages[curr_circle]
            if msg then
                self.circle_transition = {
                    title = msg.title,
                    subtitle = msg.subtitle,
                    accent = msg.accent,
                    timer = 2.0,
                    duration = 2.0,
                }
            end
        end
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
        return dx * dx + dy * dy < self.map.exit.radius * self.map.exit.radius
    elseif completion == "kill_all" then
        return not self.map:enemies_alive()
    end

    return false
end

function FPSMode:trigger_hard_cut()
    if self.hard_cut_pending then return end
    self.hard_cut_pending = true

    local deaths = self.deaths
    local time = self.time
    local timer = self.rooms[self.room_index].trial_timer or 20
    local time_ratio = time / timer

    local grade, points
    if deaths == 0 and time_ratio < 0.5 then grade, points = "S", 300
    elseif deaths == 0 and time_ratio < 0.75 then grade, points = "A", 200
    elseif deaths <= 1 then grade, points = "B", 100
    else grade, points = "C", 50 end

    self.trial_score = {
        grade = grade, points = points,
        time = time, deaths = deaths,
        timer = 0.65,
    }
    self.score = self.score + points
end

function FPSMode:update(dt)
    -- Circle transition
    if self.circle_transition then
        self.circle_transition.timer = self.circle_transition.timer - dt
        if self.circle_transition.timer <= 0 then
            self.circle_transition = nil
        end
        return
    end

    -- Hard cut
    if self.hard_cut_pending then
        self.hard_cut_timer = self.hard_cut_timer + dt
        if self.trial_score then
            self.trial_score.timer = self.trial_score.timer - dt
        end
        if self.hard_cut_timer > 0.08 and (not self.trial_score or self.trial_score.timer <= 0) then
            self.hard_cut_pending = false
            self:load_room(self.room_index + 1)
        end
        return
    end

    self.time = self.time + dt

    -- Dead: respawn after brief pause
    if self.player.is_dead then
        if self.time > 0.5 then
            self.deaths = self.deaths + 1
            self.player:spawn(self.map.spawn)
            self.player.is_dead = false
            self.trial_timer = self.rooms[self.room_index].trial_timer or 20
            self.time = 0
        end
        return
    end

    self.player:update(dt, self.map)

    -- Enemy AI: move toward player and attack
    for _, sprite in ipairs(self.map.sprites) do
        if sprite.active and (sprite.type == "enemy" or sprite.type == "demon") then
            local dx = self.player.x - sprite.x
            local dy = self.player.y - sprite.y
            local dist = math.sqrt(dx * dx + dy * dy)

            if dist < 8 and dist > 0.5 then
                -- Move toward player
                local nx = dx / dist * sprite.speed * dt
                local ny = dy / dist * sprite.speed * dt

                if not self.map:is_solid(sprite.x + nx, sprite.y) then
                    sprite.x = sprite.x + nx
                end
                if not self.map:is_solid(sprite.x, sprite.y + ny) then
                    sprite.y = sprite.y + ny
                end
            end

            -- Attack
            sprite.attack_cooldown = math.max(0, (sprite.attack_cooldown or 0) - dt)
            if dist < (sprite.attack_range or 1.5) and sprite.attack_cooldown <= 0 then
                self.player:take_damage(1, self.sfx)
                sprite.attack_cooldown = 1.5
            end
        end
    end

    -- Timer
    if self.trial_active then
        self.trial_timer = self.trial_timer - dt
        if self.trial_timer <= 0 then
            self.trial_timer = self.rooms[self.room_index].trial_timer or 20
            self.player:take_damage(self.player.health, self.sfx)
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

    if self.circle_transition then
        local ct = self.circle_transition
        local alpha = 0
        if ct.timer > ct.duration - 0.3 then alpha = (ct.duration - ct.timer) / 0.3
        elseif ct.timer < 0.3 then alpha = ct.timer / 0.3
        else alpha = 1 end

        love.graphics.setColor(0, 0, 0, 1)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
        local accent = ct.accent or {0.9, 0.5, 0.2}
        love.graphics.setColor(accent[1], accent[2], accent[3], alpha)
        love.graphics.printf(ct.title, 0, RENDER_HEIGHT / 2 - 16, RENDER_WIDTH, "center")
        love.graphics.setColor(0.8, 0.82, 0.9, alpha * 0.7)
        love.graphics.printf(ct.subtitle, 0, RENDER_HEIGHT / 2 + 4, RENDER_WIDTH, "center")
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

    -- Exit beacon
    if self.map.exit then
        local dx = self.map.exit.x - self.player.x
        local dy = self.map.exit.y - self.player.y
        local dist = math.sqrt(dx * dx + dy * dy)
        local exit_angle = math.atan2(dy, dx) - self.player.angle
        while exit_angle > math.pi do exit_angle = exit_angle - math.pi * 2 end
        while exit_angle < -math.pi do exit_angle = exit_angle + math.pi * 2 end

        if math.abs(exit_angle) < math.pi / 3 and dist < 12 then
            local screen_x = (exit_angle / (math.pi / 3) + 0.5) * RENDER_WIDTH
            local pulse = 0.5 + 0.5 * math.sin(self.time * 4)
            love.graphics.setColor(0.9, 0.7, 0.2, 0.4 * pulse / math.max(1, dist))
            love.graphics.circle("fill", screen_x, RENDER_HEIGHT / 2, 20 / dist)
        end
    end

    -- Weapon
    self.raycaster:draw_weapon(self.player.weapon_state, self.player.move_time)
    self.raycaster:draw_crosshair()

    -- HUD
    local room = self.rooms[self.room_index]

    -- Health
    love.graphics.setColor(0.9, 0.3, 0.2, 1)
    love.graphics.printf(
        string.format("VITAE %d/%d", self.player.health, self.player.max_health),
        8, RENDER_HEIGHT - 16, 120, "left"
    )

    -- Title
    love.graphics.setColor(0.85, 0.88, 0.95, 0.8)
    love.graphics.printf(room.title or "", 0, 6, RENDER_WIDTH, "center")

    -- Timer
    if self.trial_active and self.trial_timer > 0 then
        local ratio = self.trial_timer / (room.trial_timer or 20)
        local r = ratio < 0.3 and 1 or 0.85
        local g = ratio < 0.3 and 0.2 or 0.88
        local b = ratio < 0.3 and 0.15 or 0.95
        love.graphics.setColor(r, g, b, 1)
        love.graphics.printf(string.format("%.1f", self.trial_timer), 0, 18, RENDER_WIDTH, "center")
    end

    -- Trial rule
    if self.time < 2.5 and self.trial_rule ~= "" then
        local alpha = self.time < 0.3 and self.time / 0.3 or (self.time > 2 and (2.5 - self.time) / 0.5 or 1)
        love.graphics.setColor(1, 1, 1, alpha)
        love.graphics.printf(self.trial_rule, 0, RENDER_HEIGHT / 2 - 30, RENDER_WIDTH, "center")
    end

    -- Score
    if self.score > 0 then
        love.graphics.setColor(0.7, 0.72, 0.8, 0.6)
        love.graphics.printf(tostring(self.score), RENDER_WIDTH - 60, RENDER_HEIGHT - 16, 52, "right")
    end

    -- Damage flash
    if self.player.damage_flash > 0 then
        love.graphics.setColor(0.8, 0.1, 0.05, self.player.damage_flash * 0.35)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
    end

    -- Death screen
    if self.player.is_dead then
        love.graphics.setColor(0.6, 0.05, 0.02, 0.6)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
        love.graphics.setColor(1, 0.9, 0.8, 1)
        love.graphics.printf("DEATH", 0, RENDER_HEIGHT / 2 - 10, RENDER_WIDTH, "center")
    end

    -- Hard cut
    if self.hard_cut_pending then
        love.graphics.setColor(0, 0, 0, 1)
        love.graphics.rectangle("fill", 0, 0, RENDER_WIDTH, RENDER_HEIGHT)
        if self.trial_score and self.trial_score.timer > 0 then
            local accent = {0.92, 0.55, 0.18}
            love.graphics.setColor(accent[1], accent[2], accent[3], 1)
            love.graphics.printf(self.trial_score.grade, 0, RENDER_HEIGHT / 2 - 20, RENDER_WIDTH, "center")
            love.graphics.setColor(0.8, 0.82, 0.9, 0.7)
            love.graphics.printf(string.format("+%d", self.trial_score.points), 0, RENDER_HEIGHT / 2, RENDER_WIDTH, "center")
        end
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
    if button == 1 then
        self.player:fire(self.map, self.sfx)
    end
end

return FPSMode
