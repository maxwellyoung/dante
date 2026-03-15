-- effects.lua
-- Manages all particle effects.

local Effects = {}
Effects.__index = Effects

function Effects:new()
    local class = self or Effects
    local instance = setmetatable({}, class)
    instance.particles = {}
    return instance
end

function Effects:update(dt)
    for i = #self.particles, 1, -1 do
        local p = self.particles[i]
        p.x = p.x + p.vx * dt
        p.y = p.y + p.vy * dt
        p.lifetime = p.lifetime - dt
        p.vx = p.vx * (1 - (p.drag or 0))
        p.vy = p.vy * (1 - (p.drag or 0))

        if p.lifetime <= 0 then
            table.remove(self.particles, i)
        end
    end
end

function Effects:draw()
    for _, p in ipairs(self.particles) do
        if p.lifetime > 0 and p.max_lifetime > 0 then
            local alpha = math.max(0, p.lifetime / p.max_lifetime)
            love.graphics.setColor(p.color[1], p.color[2], p.color[3], alpha)
            love.graphics.circle("fill", p.x, p.y, p.size * alpha)
        end
    end
    love.graphics.setColor(1,1,1,1)
end

function Effects:reset()
    self.particles = {}
end

function Effects:spawn(type, x, y, options)
    options = options or {}
    if type == 'land_dust' then
        self:land_dust(x, y)
    elseif type == 'wall_slide_dust' then
        self:wall_slide_dust(x, y, options.dir)
    elseif type == 'muzzle_flash' then
        self:muzzle_flash(x, y, options.dir)
    elseif type == 'sparks' then
        self:sparks(x, y)
    elseif type == 'ricochet_sparks' then
        self:ricochet_sparks(x, y)
    elseif type == 'dash_burst' then
        self:dash_burst(x, y, options.dir_x or 1, options.dir_y or 0)
    elseif type == 'dash_impact' then
        self:dash_impact(x, y, options.kind)
    elseif type == 'blood' then
        self:blood(x, y)
    elseif type == 'run_dust' then
        self:run_dust(x, y)
    elseif type == 'jump_dust' then
        self:jump_dust(x, y)
    elseif type == 'grapple_latch' then
        self:grapple_latch(x, y)
    elseif type == 'stomp_ring' then
        self:stomp_ring(x, y)
    elseif type == 'wall_jump_burst' then
        self:wall_jump_burst(x, y, options.dir or 1)
    elseif type == 'ricochet_kill' then
        self:ricochet_kill(x, y)
    end
end

function Effects:muzzle_flash(x, y, dir)
    for _ = 1, 15 do
        local angle = (dir > 0 and 0 or math.pi) + math.rad(math.random(-45, 45))
        local speed = math.random(150, 400)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.2,
            max_lifetime = 0.2,
            size = math.random(2, 5),
            color = {1, math.random(0.8, 1), 0.3}
        })
    end
end

function Effects:sparks(x, y)
    for _ = 1, 10 do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(50, 200)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.4,
            max_lifetime = 0.4,
            size = math.random(1, 3),
            color = {1, 1, 0.5}
        })
    end
end

function Effects:ricochet_sparks(x, y)
    for _ = 1, 16 do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(120, 280)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.25 + 0.12,
            max_lifetime = 0.37,
            size = math.random(2, 4),
            color = {1, 0.78, 0.38},
            drag = 0.03
        })
    end
end

function Effects:dash_burst(x, y, dir_x, dir_y)
    for _ = 1, 14 do
        local angle = math.atan(dir_y, dir_x) + math.rad(math.random(-28, 28)) + math.pi
        local speed = math.random(90, 260)
        table.insert(self.particles, {
            x = x + (math.random() - 0.5) * 10,
            y = y + (math.random() - 0.5) * 10,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.16 + 0.08,
            max_lifetime = 0.24,
            size = math.random(2, 4),
            color = {1, 0.82, 0.45},
            drag = 0.03
        })
    end
end

function Effects:dash_impact(x, y, kind)
    local count = kind == "wall" and 18 or 12
    local speed_min = kind == "wall" and 140 or 90
    local speed_max = kind == "wall" and 320 or 220
    for _ = 1, count do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(speed_min, speed_max)
        table.insert(self.particles, {
            x = x,
            y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.18 + 0.08,
            max_lifetime = 0.26,
            size = math.random(2, 5),
            color = kind == "wall" and {1, 0.9, 0.55} or {0.95, 0.75, 0.48},
            drag = 0.05
        })
    end
end

function Effects:blood(x, y)
    for _ = 1, 20 do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(100, 300)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.6,
            max_lifetime = 0.6,
            size = math.random(2, 4),
            color = {1, 0, 0},
            drag = 0.05
        })
    end
end

function Effects:land_dust(x, y)
    for _ = 1, 20 do
        local angle = math.pi + math.random() * math.pi
        local speed = math.random(50, 150)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.5 + 0.3,
            max_lifetime = 0.8,
            size = math.random(1, 4),
            color = {1, 1, 1}
        })
    end
end

function Effects:wall_slide_dust(x, y, dir)
    for _ = 1, 3 do
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.random(20, 50) * dir,
            vy = math.random(-20, 20),
            lifetime = math.random() * 0.3,
            max_lifetime = 0.3,
            size = math.random(1, 3),
            color = {1, 1, 1}
        })
    end
end

function Effects:run_dust(x, y)
    table.insert(self.particles, {
        x = x, y = y,
        vx = (math.random() - 0.5) * 50,
        vy = math.random(-20, -10),
        lifetime = math.random() * 0.3,
        max_lifetime = 0.3,
        size = math.random(1, 3),
        color = {0.8, 0.7, 0.6}
    })
end

function Effects:jump_dust(x, y)
    for _ = 1, 15 do
        local angle = math.pi + math.random() * math.pi
        local speed = math.random(20, 120)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.5,
            max_lifetime = 0.5,
            size = math.random(2, 4),
            color = {0.9, 0.8, 0.7}
        })
    end
end

function Effects:stomp(x, y)
    -- Enemy stomp effect - burst of particles
    for _ = 1, 25 do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(80, 250)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed - 50, -- Slight upward bias
            lifetime = math.random() * 0.4 + 0.2,
            max_lifetime = 0.6,
            size = math.random(2, 5),
            color = {1, 0.8, 0.3},
            drag = 0.03
        })
    end
end

function Effects:grapple_latch(x, y)
    -- Tight radial snap burst — white/blue tension lines
    for _ = 1, 20 do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(200, 450)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.12 + 0.06,
            max_lifetime = 0.18,
            size = math.random(2, 4),
            color = {0.7, 0.85, 1},
            drag = 0.06
        })
    end
    -- Central flash
    table.insert(self.particles, {
        x = x, y = y,
        vx = 0, vy = 0,
        lifetime = 0.1,
        max_lifetime = 0.1,
        size = 12,
        color = {1, 1, 1},
        drag = 0
    })
end

function Effects:stomp_ring(x, y)
    -- Radial ground ring — orange/yellow spreading outward
    for i = 1, 24 do
        local angle = (i / 24) * 2 * math.pi
        local speed = math.random(160, 340)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed * 0.4 - 40,
            lifetime = math.random() * 0.3 + 0.15,
            max_lifetime = 0.45,
            size = math.random(3, 6),
            color = {1, 0.85, 0.3},
            drag = 0.04
        })
    end
end

function Effects:wall_jump_burst(x, y, dir)
    -- Directional kick-off dust opposite to jump direction
    for _ = 1, 16 do
        local angle = (dir > 0 and math.pi or 0) + math.rad(math.random(-35, 35))
        local speed = math.random(80, 220)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed - 30,
            lifetime = math.random() * 0.3 + 0.1,
            max_lifetime = 0.4,
            size = math.random(2, 5),
            color = {0.85, 0.8, 0.7},
            drag = 0.03
        })
    end
end

function Effects:ricochet_kill(x, y)
    -- Bright trick-shot flash — gold/white explosion
    for _ = 1, 30 do
        local angle = math.random() * 2 * math.pi
        local speed = math.random(100, 380)
        table.insert(self.particles, {
            x = x, y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.35 + 0.15,
            max_lifetime = 0.5,
            size = math.random(3, 6),
            color = {1, 0.92, 0.4},
            drag = 0.03
        })
    end
    -- Big central flash
    table.insert(self.particles, {
        x = x, y = y,
        vx = 0, vy = 0,
        lifetime = 0.15,
        max_lifetime = 0.15,
        size = 16,
        color = {1, 1, 0.85},
        drag = 0
    })
end

function Effects:rumble(x, y)
    -- Ground impact effect - debris flying up
    for _ = 1, 12 do
        local angle = -math.pi/2 + (math.random() - 0.5) * math.pi * 0.6
        local speed = math.random(60, 180)
        table.insert(self.particles, {
            x = x + (math.random() - 0.5) * 20,
            y = y,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            lifetime = math.random() * 0.4 + 0.3,
            max_lifetime = 0.7,
            size = math.random(2, 4),
            color = {0.6, 0.5, 0.4},
            drag = 0.02
        })
    end
end

return Effects
