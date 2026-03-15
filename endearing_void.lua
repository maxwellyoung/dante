-- endearing_void.lua
-- ENDEARING VOID — A first-person exploration narrative.
-- Walter Neistat wakes up in Paris. Then the stars.
--
-- Glitched Games — Maxwell Young & Adam Van der Voorn
-- Wellington, New Zealand

local Raycaster = require("raycaster")
local FPSMap = require("fps_map")
local Sfx = require("sfx")

local EV = {}
EV.__index = EV

local W = 320
local H = 200

-- Colors: warm, golden, Wes Anderson palette
local PALETTE = {
    gold = { 0.92, 0.78, 0.45 },
    cream = { 0.95, 0.92, 0.82 },
    burgundy = { 0.55, 0.15, 0.18 },
    navy = { 0.1, 0.12, 0.2 },
    forest = { 0.18, 0.28, 0.15 },
    rust = { 0.7, 0.35, 0.2 },
    warm_white = { 0.98, 0.95, 0.88 },
    night = { 0.04, 0.05, 0.1 },
    star = { 0.95, 0.92, 0.8 },
}

-- ============================================================
-- GAME STATES
-- ============================================================

function EV:new()
    local instance = setmetatable({}, self)
    instance.canvas = love.graphics.newCanvas(W, H)
    instance.canvas:setFilter("nearest", "nearest")
    instance.raycaster = Raycaster:new(W, H)
    instance.sfx = Sfx:new()
    instance.time = 0
    instance.state = "title"  -- title, car, hotel_exterior, lobby, hallway, room, bed, stars
    instance.state_time = 0
    instance.fade_alpha = 1
    instance.fade_target = 0
    instance.fade_speed = 1.5

    -- Player
    instance.px = 2.5
    instance.py = 2.5
    instance.angle = 0
    instance.move_time = 0

    -- Map
    instance.map = nil

    -- Room tasks
    instance.tasks = {
        lightbulb = false,
        unpack = false,
        candles = false,
        cigarettes = false,
        bed = false,
    }
    instance.tasks_done = 0
    instance.total_tasks = 5

    -- Interaction
    instance.interact_prompt = nil
    instance.interact_target = nil

    -- Title planet
    instance.planet_angle_x = 0
    instance.planet_angle_y = 0

    -- Ambient
    instance.ambient = nil
    -- Dithering
    instance.dither = true

    -- Suitcase picked up
    instance.has_suitcase = false

    -- Screen text
    instance.screen_text = nil
    instance.screen_text_timer = 0

    return instance
end

function EV:generate_ambient(tone, vol)
    if self.ambient then self.ambient:stop() end
    local sr = 44100
    local dur = 4.0
    local count = math.floor(sr * dur)
    local data = love.sound.newSoundData(count, sr, 16, 1)
    for i = 0, count - 1 do
        local t = i / sr
        local env = 1
        local fade = 0.3 * dur
        if t < fade then env = t / fade
        elseif t > dur - fade then env = (dur - t) / fade end
        local s = math.sin(t * tone * math.pi * 2) * 0.3
               + math.sin(t * (tone * 1.5 + 0.2) * math.pi * 2) * 0.15
               + math.sin(t * (tone * 2.01) * math.pi * 2) * 0.05
        data:setSample(i, math.max(-1, math.min(1, s * vol * env)))
    end
    local source = love.audio.newSource(data)
    source:setLooping(true)
    source:setVolume(vol)
    source:play()
    self.ambient = source
end

function EV:transition_to(state)
    self.fade_target = 1
    self.fade_speed = 2
    self._next_state = state
end

function EV:load_state(state)
    self.state = state
    self.state_time = 0
    self.interact_prompt = nil
    self.interact_target = nil
    self.screen_text = nil
    self.screen_text_timer = 0

    if state == "title" then
        love.mouse.setRelativeMode(false)
        self:generate_ambient(220, 0.008)
    elseif state == "car" then
        love.mouse.setRelativeMode(false)
        self:generate_ambient(110, 0.01)
        self.screen_text = "You wake up."
        self.screen_text_timer = 3
    elseif state == "hotel_exterior" then
        love.mouse.setRelativeMode(false)
        self.has_suitcase = false
        self:generate_ambient(165, 0.008)
    elseif state == "driving" then
        -- Driving the Fiat through Paris at night
        love.mouse.setRelativeMode(false)
        self:generate_ambient(146, 0.01)
        self.drive_time = 0
        self.drive_speed = 0
        self.screen_text = "Hold W to drive."
        self.screen_text_timer = 3
    elseif state == "lobby" then
        love.mouse.setRelativeMode(true)
        love.mouse.setGrabbed(true)
        self:generate_ambient(130, 0.01)
        self.raycaster.fog_color = { 0.05, 0.04, 0.02 }
        self.raycaster.floor_color = { 0.4, 0.3, 0.15 }
        self.raycaster.ceiling_color = { 0.22, 0.17, 0.1 }
        self.raycaster.max_depth = 20
        self.raycaster.fog_start = 10
        -- Gehry/brutalist lobby: concrete + gold + eco accents
        self.raycaster.wall_colors[1] = { 0.55, 0.55, 0.58 }  -- concrete
        self.raycaster.wall_colors[2] = { 0.75, 0.62, 0.35 }  -- gold trim
        self.raycaster.wall_colors[3] = { 0.15, 0.35, 0.2 }   -- plant walls
        self.raycaster.wall_colors[4] = { 0.75, 0.68, 0.45 }  -- titanium
        -- Grand lobby with angular Gehry pillars and asymmetric layout
        self.map = FPSMap:new({
            grid = {
                "111111111111111111111111111",
                "1.......................1.1",
                "1..44...................1.1",
                "1..44.......33..........1",
                "1...........33..........1",
                "1.......................1",
                "1.......22..........44..1",
                "1.......22..............1",
                "1.......................1",
                "1...44..............33..1",
                "1.......................X1",
                "1P..........................1",
                "11111111111111111111111111111",
            },
            spawn_angle = -1.57,
        })
        self.px = self.map.spawn.x
        self.py = self.map.spawn.y
        self.angle = self.map.spawn.angle or 0
        self.screen_text = "The lobby."
        self.screen_text_timer = 2.5
    elseif state == "hallway" then
        love.mouse.setRelativeMode(true)
        -- Long hallway with alternating brutalist concrete and Godard red/blue doors
        self.raycaster.wall_colors[1] = { 0.5, 0.5, 0.52 }   -- concrete
        self.raycaster.wall_colors[2] = { 0.75, 0.6, 0.35 }   -- gold
        self.raycaster.wall_colors[7] = { 0.8, 0.2, 0.15 }    -- red door
        self.raycaster.wall_colors[8] = { 0.15, 0.3, 0.7 }    -- blue door
        self.map = FPSMap:new({
            grid = {
                "111111111111111111111111111111111",
                "1P....7..1..8..1..7..1..8......X1",
                "1...............................1",
                "111111111111111111111111111111111",
            },
            spawn_angle = 0,
        })
        self.px = self.map.spawn.x
        self.py = self.map.spawn.y
        self.angle = 0
    elseif state == "room" then
        love.mouse.setRelativeMode(true)
        self:generate_ambient(98, 0.008)
        self.raycaster.fog_color = { 0.04, 0.035, 0.02 }
        self.raycaster.floor_color = { 0.3, 0.22, 0.12 }
        self.raycaster.ceiling_color = { 0.18, 0.14, 0.08 }
        self.raycaster.wall_colors[1] = { 0.6, 0.48, 0.3 }
        self.raycaster.wall_colors[2] = { 0.72, 0.58, 0.35 }
        -- Hotel room: bed area, desk area, balcony door, bathroom
        -- Interactive objects as sprites
        self.map = FPSMap:new({
            grid = {
                "111111111111111",
                "1.............1",
                "1.............1",
                "1....11.......1",
                "1....11.......1",
                "1.............1",
                "1.............1",
                "1.......11....1",
                "1.......11....1",
                "1.............1",
                "1.............1",
                "1P............1",
                "111111111111111",
            },
            spawn_angle = -1.57,
        })
        -- Add interactive objects
        self.map.sprites = {
            { x = 3.5, y = 2.5, type = "object", name = "lightbulb", task = "lightbulb",
              color = PALETTE.gold, active = true, has_face = false, width_ratio = 0.4,
              prompt = "Change the lightbulb", done_text = "Better." },
            { x = 12.5, y = 2.5, type = "object", name = "drawers", task = "unpack",
              color = PALETTE.rust, active = true, has_face = false, width_ratio = 0.8,
              prompt = "Unpack your clothes", done_text = "Home for now." },
            { x = 7.5, y = 5.5, type = "object", name = "candles", task = "candles",
              color = PALETTE.cream, active = true, has_face = false, width_ratio = 0.3,
              prompt = "Light the candles", done_text = "Warm." },
            { x = 12.5, y = 8.5, type = "object", name = "ashtray", task = "cigarettes",
              color = { 0.5, 0.45, 0.4 }, active = true, has_face = false, width_ratio = 0.4,
              prompt = "Clear the cigarettes", done_text = "Clean air." },
            { x = 3.5, y = 8.5, type = "object", name = "bed", task = "bed",
              color = PALETTE.cream, active = true, has_face = false, width_ratio = 1.0,
              prompt = "Make the bed", done_text = "Smooth." },
        }
        self.px = self.map.spawn.x
        self.py = self.map.spawn.y
        self.angle = self.map.spawn.angle or 0
        self.tasks = { lightbulb = false, unpack = false, candles = false, cigarettes = false, bed = false }
        self.tasks_done = 0
        self.screen_text = "Make this place yours."
        self.screen_text_timer = 3
    elseif state == "bed" then
        love.mouse.setRelativeMode(false)
        self:generate_ambient(82, 0.006)
    elseif state == "stars" then
        love.mouse.setRelativeMode(false)
        self:generate_ambient(55, 0.005)
    end

    self.fade_alpha = 1
    self.fade_target = 0
    self.fade_speed = 1.2
end

-- ============================================================
-- UPDATE
-- ============================================================

function EV:update(dt)
    self.time = self.time + dt
    self.state_time = self.state_time + dt

    -- Fade
    if self.fade_alpha ~= self.fade_target then
        local dir = self.fade_target > self.fade_alpha and 1 or -1
        self.fade_alpha = self.fade_alpha + dir * self.fade_speed * dt
        if (dir > 0 and self.fade_alpha >= self.fade_target) or
           (dir < 0 and self.fade_alpha <= self.fade_target) then
            self.fade_alpha = self.fade_target
            if self._next_state then
                self:load_state(self._next_state)
                self._next_state = nil
            end
        end
    end

    -- Screen text
    if self.screen_text_timer > 0 then
        self.screen_text_timer = self.screen_text_timer - dt
        if self.screen_text_timer <= 0 then self.screen_text = nil end
    end

    self.raycaster:update_shake(dt)

    if self.state == "title" then
        -- Planet rotation with mouse/keys
        if love.keyboard.isDown("left", "a") then self.planet_angle_y = self.planet_angle_y - dt * 0.8 end
        if love.keyboard.isDown("right", "d") then self.planet_angle_y = self.planet_angle_y + dt * 0.8 end
        if love.keyboard.isDown("up", "w") then self.planet_angle_x = self.planet_angle_x - dt * 0.5 end
        if love.keyboard.isDown("down", "s") then self.planet_angle_x = self.planet_angle_x + dt * 0.5 end

    elseif self.state == "car" then
        if self.state_time > 4 then
            self:transition_to("driving")
        end

    elseif self.state == "driving" then
        self.drive_time = (self.drive_time or 0) + dt
        -- Hold W to drive
        if love.keyboard.isDown("w", "up") then
            self.drive_speed = math.min(1, (self.drive_speed or 0) + dt * 0.5)
        else
            self.drive_speed = math.max(0, (self.drive_speed or 0) - dt * 0.8)
        end
        -- Arrive at hotel after driving for 8 seconds total
        if self.drive_time > 10 then
            self:transition_to("hotel_exterior")
        end

    elseif self.state == "hotel_exterior" then
        if self.state_time > 1 and not self.has_suitcase then
            self.has_suitcase = true
            self.screen_text = "You pick up your suitcase."
            self.screen_text_timer = 2.5
        end
        if self.state_time > 5 then
            self:transition_to("lobby")
        end

    elseif self.state == "lobby" or self.state == "hallway" or self.state == "room" then
        self:update_fps(dt)

    elseif self.state == "bed" then
        if self.state_time > 5 then
            self:transition_to("stars")
        end

    elseif self.state == "stars" then
        -- Hold on stars
    end
end

function EV:update_fps(dt)
    -- Movement
    local speed = 2.5
    local cos_a = math.cos(self.angle)
    local sin_a = math.sin(self.angle)
    local mx, my = 0, 0
    local moving = false

    if love.keyboard.isDown("w", "up") then mx = mx + cos_a; my = my + sin_a; moving = true end
    if love.keyboard.isDown("s", "down") then mx = mx - cos_a; my = my - sin_a; moving = true end
    if love.keyboard.isDown("a") then mx = mx + sin_a; my = my - cos_a; moving = true end
    if love.keyboard.isDown("d") then mx = mx - sin_a; my = my + cos_a; moving = true end

    if moving then self.move_time = self.move_time + dt end

    local len = math.sqrt(mx * mx + my * my)
    if len > 0 then
        mx = mx / len * speed * dt
        my = my / len * speed * dt
    end

    local nx = self.px + mx
    local ny = self.py + my
    local r = 0.25
    if self.map and not self.map:is_solid(nx + r, self.py) and not self.map:is_solid(nx - r, self.py) then
        self.px = nx
    end
    if self.map and not self.map:is_solid(self.px, ny + r) and not self.map:is_solid(self.px, ny - r) then
        self.py = ny
    end

    -- Footsteps
    if moving then
        self.footstep_timer = (self.footstep_timer or 0) - dt
        if self.footstep_timer <= 0 then
            self.sfx:play("footstep")
            self.footstep_timer = 0.45
        end
    end

    -- Check exit
    if self.map and self.map.exit then
        local dx = self.px - self.map.exit.x
        local dy = self.py - self.map.exit.y
        if dx * dx + dy * dy < 0.5 then
            if self.state == "lobby" then
                self:transition_to("hallway")
            elseif self.state == "hallway" then
                self:transition_to("room")
            end
        end
    end

    -- Interaction check (room state)
    if self.state == "room" and self.map then
        self.interact_prompt = nil
        self.interact_target = nil
        for _, sprite in ipairs(self.map.sprites) do
            if sprite.active and sprite.type == "object" and not self.tasks[sprite.task] then
                local dx = sprite.x - self.px
                local dy = sprite.y - self.py
                if dx * dx + dy * dy < 2.5 then
                    -- Check if looking at it
                    local sa = math.atan2(dy, dx) - self.angle
                    while sa > math.pi do sa = sa - math.pi * 2 end
                    while sa < -math.pi do sa = sa + math.pi * 2 end
                    if math.abs(sa) < 0.5 then
                        self.interact_prompt = sprite.prompt
                        self.interact_target = sprite
                    end
                end
            end
        end
    end
end

-- ============================================================
-- DRAW
-- ============================================================

function EV:draw()
    local ww = love.graphics.getWidth()
    local wh = love.graphics.getHeight()

    love.graphics.setCanvas(self.canvas)
    love.graphics.clear(0, 0, 0, 1)

    if self.state == "title" then
        self:draw_title()
    elseif self.state == "car" then
        self:draw_car()
    elseif self.state == "driving" then
        self:draw_driving()
    elseif self.state == "hotel_exterior" then
        self:draw_hotel_exterior()
    elseif self.state == "lobby" or self.state == "hallway" or self.state == "room" then
        self:draw_fps()
    elseif self.state == "bed" then
        self:draw_bed()
    elseif self.state == "stars" then
        self:draw_stars()
    end

    -- Dithering post-process
    if self.dither then
        self.raycaster.dither_enabled = true
        self.raycaster:apply_dither()
    end

    -- Fade overlay
    if self.fade_alpha > 0 then
        love.graphics.setColor(0, 0, 0, self.fade_alpha)
        love.graphics.rectangle("fill", 0, 0, W, H)
    end

    -- Screen text
    if self.screen_text and self.screen_text_timer > 0 then
        local a = math.min(1, self.screen_text_timer / 0.5)
        love.graphics.setColor(0, 0, 0, 0.35 * a)
        love.graphics.rectangle("fill", 0, H - 36, W, 28)
        love.graphics.setColor(PALETTE.cream[1], PALETTE.cream[2], PALETTE.cream[3], a)
        love.graphics.printf(self.screen_text, 12, H - 32, W - 24, "left")
    end

    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.setCanvas()

    local sx = self.raycaster.shake_x * (ww / W)
    local sy = self.raycaster.shake_y * (wh / H)
    love.graphics.draw(self.canvas, sx, sy, 0, ww / W, wh / H)
end

function EV:draw_title()
    -- Night sky background
    love.graphics.setColor(PALETTE.night[1], PALETTE.night[2], PALETTE.night[3], 1)
    love.graphics.rectangle("fill", 0, 0, W, H)

    -- Stars
    math.randomseed(42)
    for _ = 1, 80 do
        local sx = math.random(0, W)
        local sy = math.random(0, H)
        local brightness = 0.3 + math.random() * 0.7
        local twinkle = 0.7 + 0.3 * math.sin(self.time * (1 + math.random() * 2) + math.random() * 10)
        love.graphics.setColor(PALETTE.star[1], PALETTE.star[2], PALETTE.star[3], brightness * twinkle)
        love.graphics.rectangle("fill", sx, sy, 1, 1)
    end
    math.randomseed(os.time())

    -- Planet/moon — simple circle with rotation-dependent shading
    local cx, cy = W / 2, H / 2 + 20
    local radius = 40
    local phase = self.planet_angle_y

    -- Planet body
    love.graphics.setColor(0.25, 0.22, 0.3, 1)
    love.graphics.circle("fill", cx, cy, radius)

    -- Surface detail (craters as darker circles, position shifts with rotation)
    for i = 1, 8 do
        local crater_base_x = math.sin(i * 1.7 + phase) * radius * 0.6
        local crater_base_y = math.cos(i * 2.3 + self.planet_angle_x) * radius * 0.4
        local crater_r = 3 + (i % 3) * 2
        local dist = math.sqrt(crater_base_x * crater_base_x + crater_base_y * crater_base_y)
        if dist < radius - crater_r then
            love.graphics.setColor(0.18, 0.16, 0.22, 0.6)
            love.graphics.circle("fill", cx + crater_base_x, cy + crater_base_y, crater_r)
        end
    end

    -- Atmosphere rim
    love.graphics.setColor(0.4, 0.45, 0.6, 0.15)
    love.graphics.circle("line", cx, cy, radius + 2)
    love.graphics.circle("line", cx, cy, radius + 4)

    -- Shadow (day/night terminator based on rotation)
    love.graphics.setColor(0, 0, 0, 0.5)
    local shadow_offset = math.sin(phase) * radius * 0.6
    love.graphics.arc("fill", "pie", cx + shadow_offset, cy, radius, -math.pi/2, math.pi/2, 32)

    -- Title
    love.graphics.setColor(PALETTE.cream[1], PALETTE.cream[2], PALETTE.cream[3], 0.9)
    love.graphics.printf("E N D E A R I N G   V O I D", 0, 20, W, "center")

    -- Info panels based on planet rotation
    local info_index = math.floor((phase / (math.pi * 2)) * 4) % 4
    local infos = {
        "A game by Glitched Games\nMaxwell Young & Adam Van der Voorn",
        "Walter Neistat, 34\nNew York City → Paris",
        "A romance between a man\nand the final frontier",
        "Wellington, New Zealand\n2026",
    }
    love.graphics.setColor(PALETTE.cream[1], PALETTE.cream[2], PALETTE.cream[3], 0.5)
    love.graphics.printf(infos[info_index + 1], 0, H - 40, W, "center")

    -- Controls hint
    local pulse = 0.3 + 0.3 * math.sin(self.time * 2)
    love.graphics.setColor(PALETTE.gold[1], PALETTE.gold[2], PALETTE.gold[3], pulse)
    love.graphics.printf("PRESS ENTER", 0, H - 16, W, "center")

    love.graphics.setColor(1, 1, 1, 1)
end

function EV:draw_car()
    -- Dark interior, phone glow
    love.graphics.setColor(0.03, 0.03, 0.05, 1)
    love.graphics.rectangle("fill", 0, 0, W, H)

    -- Phone glow in center
    local glow = 0.5 + 0.5 * math.sin(self.state_time * 4)
    love.graphics.setColor(0.3, 0.4, 0.7, 0.15 * glow)
    love.graphics.rectangle("fill", W/2 - 30, H/2 - 20, 60, 40)

    -- Phone screen
    love.graphics.setColor(0.15, 0.2, 0.35, 0.8)
    love.graphics.rectangle("fill", W/2 - 20, H/2 - 14, 40, 28, 2, 2)
    love.graphics.setColor(0.6, 0.7, 0.9, glow)
    love.graphics.rectangle("fill", W/2 - 18, H/2 - 12, 36, 24, 1, 1)

    -- Dashboard silhouette
    love.graphics.setColor(0.06, 0.06, 0.08, 1)
    love.graphics.rectangle("fill", 0, H * 0.7, W, H * 0.3)

    -- Window: Paris at night (simple skyline)
    love.graphics.setColor(0.05, 0.06, 0.12, 1)
    love.graphics.rectangle("fill", 0, 0, W, H * 0.4)
    -- Distant lights
    for i = 1, 15 do
        local lx = (i * 21 + 7) % W
        local ly = H * 0.35 + math.sin(i * 3.1) * 8
        love.graphics.setColor(0.9, 0.75, 0.4, 0.3 + math.random() * 0.2)
        love.graphics.rectangle("fill", lx, ly, 2, 1)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function EV:draw_hotel_exterior()
    -- Night sky
    love.graphics.setColor(PALETTE.night[1], PALETTE.night[2], PALETTE.night[3], 1)
    love.graphics.rectangle("fill", 0, 0, W, H * 0.4)

    -- Hotel facade — golden, grand, centered
    local hotel_x = W * 0.15
    local hotel_w = W * 0.7
    local hotel_y = H * 0.15
    local hotel_h = H * 0.65

    -- Building body
    love.graphics.setColor(0.7, 0.58, 0.35, 1)
    love.graphics.rectangle("fill", hotel_x, hotel_y, hotel_w, hotel_h)

    -- Windows (symmetrical, Wes Anderson)
    love.graphics.setColor(0.9, 0.75, 0.4, 0.7)
    for row = 0, 3 do
        for col = 0, 4 do
            local wx = hotel_x + 12 + col * (hotel_w - 24) / 4
            local wy = hotel_y + 10 + row * (hotel_h - 20) / 4
            love.graphics.rectangle("fill", wx - 6, wy - 8, 12, 16, 1, 1)
        end
    end

    -- Entrance
    love.graphics.setColor(0.3, 0.2, 0.1, 1)
    love.graphics.rectangle("fill", W/2 - 15, hotel_y + hotel_h - 30, 30, 30)
    love.graphics.setColor(PALETTE.gold[1], PALETTE.gold[2], PALETTE.gold[3], 0.8)
    love.graphics.rectangle("line", W/2 - 15, hotel_y + hotel_h - 30, 30, 30)

    -- Light above entrance
    local entrance_glow = 0.6 + 0.2 * math.sin(self.time * 1.5)
    love.graphics.setColor(1, 0.9, 0.6, 0.3 * entrance_glow)
    love.graphics.circle("fill", W/2, hotel_y + hotel_h - 35, 20)

    -- Ground
    love.graphics.setColor(0.15, 0.14, 0.12, 1)
    love.graphics.rectangle("fill", 0, hotel_y + hotel_h, W, H - hotel_y - hotel_h)

    -- Suitcase on ground
    if not self.has_suitcase then
        love.graphics.setColor(0.5, 0.35, 0.2, 1)
        love.graphics.rectangle("fill", W/2 + 30, hotel_y + hotel_h + 5, 16, 12, 2, 2)
        -- Stickers
        love.graphics.setColor(0.2, 0.5, 0.8, 0.7)
        love.graphics.rectangle("fill", W/2 + 33, hotel_y + hotel_h + 7, 4, 3)
        love.graphics.setColor(0.8, 0.2, 0.2, 0.7)
        love.graphics.rectangle("fill", W/2 + 39, hotel_y + hotel_h + 8, 4, 3)
    end

    -- Hotel name
    love.graphics.setColor(PALETTE.gold[1], PALETTE.gold[2], PALETTE.gold[3], 0.9)
    love.graphics.printf("HOTEL CHEVALIER", hotel_x, hotel_y - 14, hotel_w, "center")

    love.graphics.setColor(1, 1, 1, 1)
end

function EV:draw_driving()
    local speed = self.drive_speed or 0
    local t = self.drive_time or 0

    -- Sky — deep blue to orange at horizon (Pierrot le Fou sunset)
    for y = 0, H * 0.45 do
        local sky_t = y / (H * 0.45)
        local r = 0.05 + sky_t * 0.3
        local g = 0.06 + sky_t * 0.12
        local b = 0.18 - sky_t * 0.08
        love.graphics.setColor(r, g, b, 1)
        love.graphics.rectangle("fill", 0, y, W, 1)
    end

    -- Stars in upper sky
    math.randomseed(42)
    for _ = 1, 30 do
        local sx = math.random(0, W)
        local sy = math.random(0, math.floor(H * 0.25))
        local bri = 0.3 + math.random() * 0.5
        love.graphics.setColor(1, 0.98, 0.9, bri * (1 - self.drive_time / 12))
        love.graphics.rectangle("fill", sx, sy, 1, 1)
    end
    math.randomseed(os.time())

    -- Buildings silhouette — brutalist blocks scrolling
    local scroll = t * speed * 40
    for i = 0, 12 do
        local bx = ((i * 28 - scroll * 0.3) % (W + 40)) - 20
        local bh = 30 + (i * 17) % 40
        local by = H * 0.45 - bh
        -- Concrete grey with occasional Godard color
        if i % 5 == 0 then
            love.graphics.setColor(0.7, 0.18, 0.12, 0.9)  -- red
        elseif i % 7 == 0 then
            love.graphics.setColor(0.12, 0.25, 0.6, 0.9)  -- blue
        else
            love.graphics.setColor(0.25, 0.25, 0.28, 0.9)  -- concrete
        end
        love.graphics.rectangle("fill", bx, by, 22, bh)
        -- Windows
        love.graphics.setColor(0.9, 0.75, 0.4, 0.4)
        for wy = 0, math.floor(bh / 8) - 1 do
            for wx = 0, 1 do
                love.graphics.rectangle("fill", bx + 4 + wx * 10, by + 4 + wy * 8, 6, 4)
            end
        end
    end

    -- Road — perspective lines converging to center
    local horizon_y = H * 0.45
    love.graphics.setColor(0.12, 0.12, 0.14, 1)
    love.graphics.rectangle("fill", 0, horizon_y, W, H - horizon_y)

    -- Road lines (scrolling)
    local line_scroll = (t * speed * 200) % 40
    for i = 0, 8 do
        local depth = (i * 40 + line_scroll) / 320
        if depth > 0.02 then
            local road_y = horizon_y + (H - horizon_y) * depth
            local line_width = 2 + depth * 20
            local cx = W / 2
            love.graphics.setColor(0.9, 0.85, 0.5, 0.4 * (1 - depth))
            love.graphics.rectangle("fill", cx - line_width / 2, road_y, line_width, 2)
        end
    end

    -- Car dashboard
    love.graphics.setColor(0.15, 0.12, 0.1, 1)
    love.graphics.rectangle("fill", 0, H * 0.78, W, H * 0.22)
    -- Steering wheel hint
    love.graphics.setColor(0.25, 0.2, 0.18, 1)
    love.graphics.circle("line", W / 2, H * 0.88, 18)

    -- Speedometer
    love.graphics.setColor(PALETTE.cream[1], PALETTE.cream[2], PALETTE.cream[3], 0.6)
    love.graphics.printf(string.format("%d km/h", math.floor(speed * 80)), W - 70, H - 18, 62, "right")

    love.graphics.setColor(1, 1, 1, 1)
end

function EV:draw_fps()
    if not self.map then return end

    local rays = self.raycaster:cast_walls(self.map, self.px, self.py, self.angle)
    self.raycaster:draw_scene(rays)
    self.raycaster:draw_sprites(self.map.sprites, self.px, self.py, self.angle)
    self.raycaster:draw_world_text(self.map.world_texts or {}, self.px, self.py, self.angle)

    -- Exit beacon
    if self.map.exit then
        local dx = self.map.exit.x - self.px
        local dy = self.map.exit.y - self.py
        local dist = math.sqrt(dx * dx + dy * dy)
        local ea = math.atan2(dy, dx) - self.angle
        while ea > math.pi do ea = ea - math.pi * 2 end
        while ea < -math.pi do ea = ea + math.pi * 2 end
        if math.abs(ea) < math.pi / 3 and dist < 10 then
            local scx = (ea / (math.pi / 3) + 0.5) * W
            local p = 0.2 + 0.15 * math.sin(self.time * 2)
            love.graphics.setColor(PALETTE.gold[1], PALETTE.gold[2], PALETTE.gold[3], p / math.max(1, dist))
            love.graphics.circle("fill", scx, H / 2, 10 / dist)
        end
    end

    -- Interaction prompt
    if self.interact_prompt then
        love.graphics.setColor(0, 0, 0, 0.4)
        love.graphics.rectangle("fill", W/2 - 80, H/2 + 20, 160, 20, 3, 3)
        love.graphics.setColor(PALETTE.cream[1], PALETTE.cream[2], PALETTE.cream[3], 0.9)
        love.graphics.printf("[E] " .. self.interact_prompt, W/2 - 75, H/2 + 24, 150, "center")
    end

    -- Task counter (room only)
    if self.state == "room" then
        love.graphics.setColor(PALETTE.gold[1], PALETTE.gold[2], PALETTE.gold[3], 0.5)
        love.graphics.printf(string.format("%d/%d", self.tasks_done, self.total_tasks), W - 50, 8, 42, "right")
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function EV:draw_bed()
    -- Looking up at ceiling
    love.graphics.setColor(0.12, 0.1, 0.07, 1)
    love.graphics.rectangle("fill", 0, 0, W, H)

    -- Glow in the dark stars
    local star_count = math.min(20, math.floor(self.state_time * 4))
    math.randomseed(99)
    for i = 1, star_count do
        local sx = math.random(W * 0.1, W * 0.9)
        local sy = math.random(H * 0.1, H * 0.7)
        local glow = 0.4 + 0.4 * math.sin(self.time * (0.5 + i * 0.1) + i * 2)
        love.graphics.setColor(0.5, 0.9, 0.5, glow * 0.6)
        love.graphics.circle("fill", sx, sy, 2)
        love.graphics.setColor(0.5, 0.9, 0.5, glow * 0.15)
        love.graphics.circle("fill", sx, sy, 6)
    end
    math.randomseed(os.time())

    love.graphics.setColor(1, 1, 1, 1)
end

function EV:draw_stars()
    -- Real stars. Deep space.
    love.graphics.setColor(PALETTE.night[1] * 0.5, PALETTE.night[2] * 0.5, PALETTE.night[3] * 0.5, 1)
    love.graphics.rectangle("fill", 0, 0, W, H)

    -- Stars — more, brighter, deeper than the title screen
    math.randomseed(77)
    local zoom = math.min(1, self.state_time / 3)
    for _ = 1, math.floor(60 + zoom * 140) do
        local sx = math.random(0, W)
        local sy = math.random(0, H)
        local brightness = (0.2 + math.random() * 0.8) * zoom
        local twinkle = 0.7 + 0.3 * math.sin(self.time * (0.5 + math.random() * 3) + math.random() * 20)
        love.graphics.setColor(1, 0.98, 0.9, brightness * twinkle)
        local size = math.random() > 0.95 and 2 or 1
        love.graphics.rectangle("fill", sx, sy, size, size)
    end
    math.randomseed(os.time())

    -- Title card
    if self.state_time > 2 then
        local title_alpha = math.min(1, (self.state_time - 2) / 2)
        love.graphics.setColor(PALETTE.cream[1], PALETTE.cream[2], PALETTE.cream[3], title_alpha * 0.9)
        love.graphics.printf("E N D E A R I N G   V O I D", 0, H / 2 - 10, W, "center")
    end

    love.graphics.setColor(1, 1, 1, 1)
end

-- ============================================================
-- INPUT
-- ============================================================

function EV:keypressed(key)
    if self.state == "title" then
        if key == "return" or key == "space" then
            self:transition_to("car")
        end
        if key == "escape" then love.event.quit() end

    elseif self.state == "car" then
        if key == "return" or key == "space" then
            self:transition_to("driving")
        end

    elseif self.state == "driving" then
        -- W drives, space/enter skips
        if key == "return" or key == "space" then
            self:transition_to("hotel_exterior")
        end

    elseif self.state == "hotel_exterior" then
        if key == "return" or key == "space" then
            self:transition_to("lobby")
        end

    elseif self.state == "room" then
        if key == "e" and self.interact_target then
            local target = self.interact_target
            self.tasks[target.task] = true
            self.tasks_done = self.tasks_done + 1
            target.active = false
            self.screen_text = target.done_text
            self.screen_text_timer = 2
            self.sfx:play("collect")
            self.interact_prompt = nil
            self.interact_target = nil

            -- All tasks done → lie on bed
            if self.tasks_done >= self.total_tasks then
                self.screen_text = "Time to rest."
                self.screen_text_timer = 2.5
                -- Delay then transition
                self._bed_timer = 3
            end
        end
        if key == "escape" then
            love.mouse.setRelativeMode(false)
            love.mouse.setGrabbed(false)
        end

    elseif self.state == "bed" then
        -- Skip to stars
        if key == "return" or key == "space" then
            self:transition_to("stars")
        end

    elseif self.state == "stars" then
        if key == "escape" then love.event.quit() end
    end
end

function EV:mousemoved(_, _, dx)
    if self.state == "lobby" or self.state == "hallway" or self.state == "room" then
        self.angle = self.angle + dx * 0.003
    elseif self.state == "title" then
        self.planet_angle_y = self.planet_angle_y + dx * 0.005
    end
end

function EV:mousepressed() end
function EV:mousereleased() end

return EV
