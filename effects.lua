Effects = {}
Effects.__index = Effects

function Effects:new(particles)
    local self = setmetatable({}, Effects)
    self.particles = particles
    return self
end

function Effects:jump(x, y)
    self.particles:add(x, y, 5, {0.5, 1, 0.5})
    -- TODO: Play jump sound
end

function Effects:land(x, y)
    self.particles:add(x, y, 8, {0.7, 0.7, 0.7})
    -- TODO: Play land sound
end

function Effects:dash(x, y)
    self.particles:add(x, y, 15, {1, 0.5, 1})
    -- TODO: Play dash sound
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