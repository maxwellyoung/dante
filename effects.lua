Effects = {}
Effects.__index = Effects

function Effects:new(particles)
    local self = setmetatable({}, Effects)
    self.particles = particles
    return self
end

function Effects:jump(x, y)
    self.particles:create_system({
        x = x, y = y,
        count = 8,
        color = {1, 1, 1, 1},
        vy_min = -100, vy_max = -20,
        vx_min = -50, vx_max = 50,
        life_max = 0.5,
        size = 4
    })
    -- TODO: Play jump sound
end

function Effects:land(x, y)
    self.particles:create_system({
        x = x, y = y,
        count = 12,
        color = {1, 1, 1, 1},
        vx_min = -120, vx_max = 120,
        vy_min = -60, vy_max = -10,
        life_max = 0.4,
        size = 5
    })
    -- TODO: Play land sound
end

function Effects:dash(x, y)
    self.particles:create_system({
        x = x, y = y,
        count = 25,
        color = {1, 0.8, 0.2, 1},
        life_max = 0.6,
        drag = 0.2,
        size = 5,
        gravity = {x = 0, y = 150}
    })
    -- TODO: Play dash sound
end

function Effects:update(dt)
    -- Effects currently have no state to update, but the function exists
    -- to prevent errors if called from main.
end

function Effects:draw()
    -- Effects are drawn via the particle system, so this can be empty.
end

function Effects:draw_ui(player)
    local abilities = {
        {name = "Double Jump", enabled = player.has_double_jump},
        {name = "Dash", enabled = player.has_dash},
        {name = "Wall Cling", enabled = player.has_wall_cling},
    }

    love.graphics.push()
    love.graphics.translate(10, 10)
    for i, ability in ipairs(abilities) do
        if ability.enabled then
            love.graphics.setColor(1, 1, 1)
        else
            love.graphics.setColor(0.5, 0.5, 0.5, 0.5)
        end
        love.graphics.print(ability.name, 0, (i-1) * 20)
    end
    love.graphics.pop()
end

return Effects 