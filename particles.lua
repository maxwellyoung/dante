Particles = {}
Particles.__index = Particles

function Particles:new()
    local self = setmetatable({}, Particles)
    self.systems = {}
    self.emitters = {} -- For persistent particle effects
    return self
end

function Particles:add_emitter(emitter)
    -- Emitter properties:
    -- x, y, width, height: The area of emission
    -- count: Max number of particles at once
    -- rate: Particles to spawn per second
    -- particle_props: { vx_min, vx_max, vy_min, vy_max, life_min, life_max, color }
    emitter.particles = {}
    emitter.spawn_timer = 0
    table.insert(self.emitters, emitter)
end

function Particles:add(x, y, amount, color)
    local system = {}
    system.particles = {}
    system.x = x
    system.y = y
    
    for i = 1, amount do
        local p = {}
        p.x = 0
        p.y = 0
        p.vx = (math.random() - 0.5) * math.random(100, 400)
        p.vy = (math.random() - 0.5) * math.random(100, 400)
        p.life = math.random() * 0.5 + 0.2
        p.color = color or {1, 1, 1, 1}
        table.insert(system.particles, p)
    end

    table.insert(self.systems, system)
end

function Particles:update(dt)
    -- Update one-shot particle systems
    for i = #self.systems, 1, -1 do
        local s = self.systems[i]
        for j = #s.particles, 1, -1 do
            local p = s.particles[j]
            p.life = p.life - dt
            if p.life <= 0 then
                table.remove(s.particles, j)
            else
                p.x = p.x + p.vx * dt
                p.y = p.y + p.vy * dt
            end
        end
        if #s.particles == 0 then
            table.remove(self.systems, i)
        end
    end

    -- Update persistent emitters
    for _, e in ipairs(self.emitters) do
        -- Update existing particles
        for i = #e.particles, 1, -1 do
            local p = e.particles[i]
            p.life = p.life - dt
            if p.life <= 0 then
                table.remove(e.particles, i)
            else
                p.x = p.x + p.vx * dt
                p.y = p.y + p.vy * dt
            end
        end

        -- Spawn new particles
        e.spawn_timer = e.spawn_timer + dt
        local spawn_interval = 1 / e.rate
        while e.spawn_timer > spawn_interval do
            e.spawn_timer = e.spawn_timer - spawn_interval
            if #e.particles < e.count then
                local p = {}
                p.x = e.x + math.random() * e.width
                p.y = e.y + math.random() * e.height
                p.vx = math.random(e.particle_props.vx_min, e.particle_props.vx_max)
                p.vy = math.random(e.particle_props.vy_min, e.particle_props.vy_max)
                p.life = math.random() * (e.particle_props.life_max - e.particle_props.life_min) + e.particle_props.life_min
                p.color = e.particle_props.color or {1, 1, 1, 1}
                table.insert(e.particles, p)
            end
        end
    end
end

function Particles:draw()
    -- Draw one-shot systems
    for _, s in ipairs(self.systems) do
        love.graphics.push()
        love.graphics.translate(s.x, s.y)
        for _, p in ipairs(s.particles) do
            love.graphics.setColor(p.color[1], p.color[2], p.color[3], p.life * 2)
            love.graphics.rectangle("fill", p.x, p.y, 2, 2)
        end
        love.graphics.pop()
    end

    -- Draw emitter particles
    for _, e in ipairs(self.emitters) do
        for _, p in ipairs(e.particles) do
            local life_max = e.particle_props.life_max or 1
            love.graphics.setColor(p.color[1], p.color[2], p.color[3], p.life / life_max * 0.5) -- Fade out
            love.graphics.rectangle("fill", p.x, p.y, 3, 3)
        end
    end

    love.graphics.setColor(1, 1, 1)
end

return Particles 