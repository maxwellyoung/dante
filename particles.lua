-- particles.lua
-- A simple, high-performance particle system.

local Particles = {}
Particles.__index = Particles

-- Helper for random floats
local function randf(min, max)
    return min + math.random() * (max - min)
end

function Particles:new()
    local self = setmetatable({}, Particles)
    self.particles = {}

    -- A 1x1 white pixel texture for drawing particles without needing an image file.
    local pixel_data = love.image.newImageData(1, 1)
    pixel_data:setPixel(0, 0, 1, 1, 1, 1)
    self.texture = love.graphics.newImage(pixel_data)
    
    self.batch = love.graphics.newSpriteBatch(self.texture, 1000, "stream")

    return self
end

function Particles:add(x, y, props)
    local p = {
        x = x,
        y = y,
        vx = props.vx or 0,
        vy = props.vy or 0,
        lifetime = props.lifetime or 1,
        initial_lifetime = props.lifetime or 1,
        size = props.size or 3,
        color = props.color or {1, 1, 1, 1},
        drag = props.drag or 0,
        gravity_y = props.gravity_y or 0,
        fades = props.fades or false
    }
    table.insert(self.particles, p)
end

function Particles:update(dt)
    for i = #self.particles, 1, -1 do
        local p = self.particles[i]
        p.vy = p.vy + p.gravity_y * dt
        p.lifetime = p.lifetime - dt
        
        if p.lifetime <= 0 then
            table.remove(self.particles, i)
        else
            p.vx = p.vx * (1 - p.drag * dt)
            p.vy = p.vy * (1 - p.drag * dt)
            p.x = p.x + p.vx * dt
            p.y = p.y + p.vy * dt
            if p.fades then
                p.color[4] = (p.lifetime / p.initial_lifetime)
            end
        end
    end
end

function Particles:draw()
    self.batch:clear()
    
    for _, p in ipairs(self.particles) do
        local alpha = p.lifetime / p.initial_lifetime
        self.batch:add(p.x, p.y, 0, p.size / 1, p.size / 1, p.size/2, p.size/2)
        self.batch:setColor(p.color[1], p.color[2], p.color[3], p.color[4] * alpha)
    end
    
    love.graphics.setBlendMode("add")
    love.graphics.draw(self.batch)
    love.graphics.setBlendMode("alpha")
    love.graphics.setColor(1, 1, 1, 1)
end

function Particles:clear()
    self.particles = {}
end

return Particles 