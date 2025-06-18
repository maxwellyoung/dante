-- particles.lua
-- A high-performance particle system using struct-of-arrays (SoA) and batched rendering.

local Particles = {}
Particles.__index = Particles

-- Constants
local DEFAULT_PARTICLE_SIZE = 3
local DEFAULT_GRAVITY = { x = 0, y = 0 }
local DEFAULT_DRAG = 0

-- Helper for random floats
local function randf(rng, min, max)
    return min + rng:random() * (max - min)
end

function Particles:new(seed)
    local self = setmetatable({}, Particles)
    self.systems = {}
    if seed then
        self.rng = love.math.newRandomGenerator(seed)
    else
        self.rng = love.math.newRandomGenerator()
    end
    
    -- A 1x1 white pixel texture for drawing particles without needing an image file.
    local pixel_data = love.image.newImageData(1, 1)
    pixel_data:setPixel(0, 0, 1, 1, 1, 1)
    self.texture = love.graphics.newImage(pixel_data)
    
    return self
end

-- The main factory for creating new particle systems.
function Particles:create_system(config)
    -- Merge user config with defaults
    local system = {
        -- Core properties
        mode = config.mode or "burst", -- "burst" or "continuous"
        count = config.count or 10,
        rate = config.rate or 10,
        x = config.x or 0,
        y = config.y or 0,
        
        -- Particle properties
        life_min = config.life_min or 0.5,
        life_max = config.life_max or 1.0,
        size = config.size or DEFAULT_PARTICLE_SIZE,
        color = config.color or {1, 1, 1, 1},
        
        -- Physics
        vx_min = config.vx_min or -100,
        vx_max = config.vx_max or 100,
        vy_min = config.vy_min or -100,
        vy_max = config.vy_max or 100,
        gravity = config.gravity or DEFAULT_GRAVITY,
        drag = config.drag or DEFAULT_DRAG,
        
        -- Custom logic
        on_particle_init = config.on_particle_init,
        
        -- Internal state
        is_active = true,
        spawn_timer = 0,
        particle_count = 0,
        
        -- Struct-of-Arrays data model
        p_x = {},
        p_y = {},
        p_vx = {},
        p_vy = {},
        p_life = {},
        p_initial_life = {},
    }

    -- Each system gets its own mesh for batched drawing.
    system.mesh = love.graphics.newMesh(system.count * 6, "triangles", "stream")
    system.mesh:setTexture(self.texture)

    setmetatable(system, {__index = self})

    -- For burst systems, spawn all particles at once.
    if system.mode == "burst" then
        for i = 1, system.count do
            system:spawn_particle()
        end
    end

    table.insert(self.systems, system)
    return system
end

function Particles:spawn_particle(system_override)
    local system = system_override or self -- Allows calling on a system object
    
    local i = system.particle_count + 1
    system.particle_count = i

    system.p_x[i] = system.x
    system.p_y[i] = system.y
    system.p_vx[i] = randf(system.rng, system.vx_min, system.vx_max)
    system.p_vy[i] = randf(system.rng, system.vy_min, system.vy_max)
    local life = randf(system.rng, system.life_min, system.life_max)
    system.p_life[i] = life
    system.p_initial_life[i] = life

    -- Allow gameplay code to customize a particle on creation
    if system.on_particle_init then
        system.on_particle_init(system, i)
    end
end

function Particles:update(dt)
    for s_idx = #self.systems, 1, -1 do
        local s = self.systems[s_idx]

        -- Update existing particles
        for i = s.particle_count, 1, -1 do
            s.p_life[i] = s.p_life[i] - dt
            
            if s.p_life[i] <= 0 then
                -- Swap-remove pattern for O(1) removal
                local last = s.particle_count
                s.p_x[i] = s.p_x[last]; s.p_y[i] = s.p_y[last]
                s.p_vx[i] = s.p_vx[last]; s.p_vy[i] = s.p_vy[last]
                s.p_life[i] = s.p_life[last]; s.p_initial_life[i] = s.p_initial_life[last]

                s.p_x[last] = nil; s.p_y[last] = nil
                s.p_vx[last] = nil; s.p_vy[last] = nil
                s.p_life[last] = nil; s.p_initial_life[last] = nil
                
                s.particle_count = s.particle_count - 1
            else
                s.p_vx[i] = s.p_vx[i] + s.gravity.x * dt
                s.p_vy[i] = s.p_vy[i] + s.gravity.y * dt
                s.p_vx[i] = s.p_vx[i] * (1 - s.drag * dt)
                s.p_vy[i] = s.p_vy[i] * (1 - s.drag * dt)
                
                s.p_x[i] = s.p_x[i] + s.p_vx[i] * dt
                s.p_y[i] = s.p_y[i] + s.p_vy[i] * dt
            end
        end

        -- Spawn new particles for continuous systems
        if s.mode == "continuous" and s.is_active then
            s.spawn_timer = s.spawn_timer + dt
            local spawn_interval = 1 / s.rate
            while s.spawn_timer > spawn_interval do
                s.spawn_timer = s.spawn_timer - spawn_interval
                if s.particle_count < s.count then
                    s:spawn_particle()
                end
            end
        end
        
        -- Remove dead systems
        if s.particle_count == 0 and (s.mode == "burst" or not s.is_active) then
            table.remove(self.systems, s_idx)
        end
    end
end

function Particles:draw()
    love.graphics.setBlendMode("add")

    for _, s in ipairs(self.systems) do
        if s.particle_count > 0 then
            local vertices = {}
            local r, g, b = s.color[1] * 255, s.color[2] * 255, s.color[3] * 255
            
            for i = 1, s.particle_count do
                local x, y = s.p_x[i], s.p_y[i]
                local size = s.size
                local alpha = (s.p_life[i] / s.p_initial_life[i]) * (s.color[4] or 1) * 255
                
                local x1, y1 = x - size/2, y - size/2
                local x2, y2 = x + size/2, y + size/2
                
                -- Triangle 1
                table.insert(vertices, {x1, y1, 0, 0, r, g, b, alpha})
                table.insert(vertices, {x2, y1, 1, 0, r, g, b, alpha})
                table.insert(vertices, {x2, y2, 1, 1, r, g, b, alpha})
                
                -- Triangle 2
                table.insert(vertices, {x1, y1, 0, 0, r, g, b, alpha})
                table.insert(vertices, {x2, y2, 1, 1, r, g, b, alpha})
                table.insert(vertices, {x1, y2, 0, 1, r, g, b, alpha})
            end
            
            s.mesh:setVertices(vertices, 1)
            love.graphics.draw(s.mesh, 0, 0)
        end
    end
    
    love.graphics.setBlendMode("alpha")
    love.graphics.setColor(1, 1, 1, 1)
end

-- Public method to stop an emitter
function Particles:stop(system)
    if system then
        system.is_active = false
    end
end

-- Clears all particles and systems. Useful for level resets.
function Particles:clear()
    self.systems = {}
end

return Particles 