local Background = {}

function Background:new()
    local self = setmetatable({}, {__index = Background})
    
    self.layers = {}
    
    return self
end

function Background:load()
    self.canvas = love.graphics.newCanvas(g_native_width, g_native_height)
    self.shader = love.graphics.newShader[[
        extern number time;
        vec4 effect(vec4 color, Image tex, vec2 tc, vec2 sc) {
            vec2 motion = vec2(sin(time * 0.2 + tc.y * 10.0), cos(time * 0.2 + tc.x * 10.0));
            vec2 uv = tc + motion * 0.05;
            float noise = Texel(tex, uv).r;
            noise = pow(noise, 1.5);
            return vec4(color.rgb * noise, color.a);
        }
    ]]
    
    -- Define layers inspired by Jeremy Blake's style
    self.layers = {
        { -- Deep, dark base
            color = {10/255, 5/255, 25/255, 1},
            speed = {x = 15, y = 10},
            scale = 4,
            blend = "alpha"
        },
        { -- Saturated magenta/purple forms
            color = {180/255, 40/255, 120/255, 0.05},
            speed = {x = -20, y = 15},
            scale = 3,
            blend = "add"
        },
        { -- Bright cyan highlights
            color = {50/255, 200/255, 220/255, 0.03},
            speed = {x = 30, y = 25},
            scale = 2.5,
            blend = "add"
        },
        { -- Slow, subtle yellow grain
            color = {255/255, 230/255, 100/255, 0.02},
            speed = {x = 5, y = 5},
            scale = 8,
            blend = "alpha"
        },
    }
end

function Background:update(dt)
    -- The shader handles the animation, but we prepare the canvas here
    love.graphics.setCanvas(self.canvas)
    love.graphics.clear(0,0,0,0) -- Clear with transparency

    love.graphics.setShader(self.shader)
    self.shader:send("time", love.timer.getTime())

    for _, layer in ipairs(self.layers) do
        love.graphics.setColor(layer.color)
        love.graphics.setBlendMode(layer.blend)
        
        local off_x = (g_camera.x * layer.speed.x / 100) % g_noise_texture:getWidth()
        local off_y = (g_camera.y * layer.speed.y / 100) % g_noise_texture:getHeight()

        love.graphics.draw(g_noise_texture, -off_x, -off_y, 0, layer.scale, layer.scale)
        love.graphics.draw(g_noise_texture, -off_x + g_noise_texture:getWidth() * layer.scale, -off_y, 0, layer.scale, layer.scale)
    end
    
    love.graphics.setBlendMode("alpha")
    love.graphics.setShader()
    love.graphics.setCanvas()
end

function Background:draw()
    -- Draw the pre-rendered canvas to the current target
    love.graphics.setColor(1,1,1,1)
    love.graphics.draw(self.canvas)
end

return Background