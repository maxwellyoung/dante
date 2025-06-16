Particles = {}
Particles.__index = Particles

function Particles:new()
    local self = setmetatable({}, Particles)
    self.systems = {}
    return self
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
end

function Particles:draw()
    for _, s in ipairs(self.systems) do
        love.graphics.push()
        love.graphics.translate(s.x, s.y)
        for _, p in ipairs(s.particles) do
            love.graphics.setColor(p.color[1], p.color[2], p.color[3], p.life * 2)
            love.graphics.rectangle("fill", p.x, p.y, 2, 2)
        end
        love.graphics.pop()
    end
    love.graphics.setColor(1, 1, 1)
end

return Particles 