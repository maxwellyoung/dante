local Background = {}
Background.__index = Background

local function clamp01(value)
    return math.max(0, math.min(1, value))
end

local function mix_color(a, b, t)
    return {
        a[1] + (b[1] - a[1]) * t,
        a[2] + (b[2] - a[2]) * t,
        a[3] + (b[3] - a[3]) * t,
    }
end

function Background:new()
    local class = self or Background
    local instance = setmetatable({}, { __index = class })
    instance.color = { 0.08, 0.09, 0.11 }
    instance.floor_color = { 0.12, 0.13, 0.16 }
    instance.accent_color = { 0.92, 0.55, 0.18 }
    instance.scene = nil
    instance.time = 0
    return instance
end

function Background:set_palette(base_color)
    self.color = {
        base_color[1],
        base_color[2],
        base_color[3],
    }
    self.floor_color = {
        clamp01(base_color[1] + 0.06),
        clamp01(base_color[2] + 0.06),
        clamp01(base_color[3] + 0.08),
    }
end

function Background:set_scene(scene)
    self.scene = scene
    self:set_palette(scene.bg or self.color)
    self.accent_color = scene.accent_color or { 0.92, 0.55, 0.18 }
end

function Background.load(_self) end

function Background:update(dt)
    self.time = self.time + dt
end

function Background:draw_gradient()
    local top = mix_color(self.color, { 0.02, 0.02, 0.03 }, 0.35)
    local bottom = mix_color(self.floor_color, self.accent_color, 0.14)

    for index = 0, g_native_height - 1, 3 do
        local t = index / g_native_height
        local color = mix_color(top, bottom, t)
        love.graphics.setColor(color[1], color[2], color[3], 1)
        love.graphics.rectangle("fill", 0, index, g_native_width, 3)
    end
end

function Background:draw_arches()
    local accent = self.accent_color
    local pulse = 0.12 + 0.03 * math.sin(self.time * 0.8)

    love.graphics.setColor(accent[1], accent[2], accent[3], pulse)
    love.graphics.circle("fill", g_native_width * 0.76, g_native_height * 0.22, 44)

    love.graphics.setColor(0.02, 0.02, 0.03, 0.22)
    for index = 0, 3 do
        local width = 110 + index * 38
        local height = 70 + index * 18
        love.graphics.arc(
            "line",
            "open",
            g_native_width * 0.72,
            g_native_height * 0.62,
            width,
            math.pi,
            math.pi * 2,
            48
        )
        love.graphics.arc(
            "line",
            "open",
            g_native_width * 0.72,
            g_native_height * 0.62,
            height,
            math.pi,
            math.pi * 2,
            48
        )
    end
end

function Background.draw_grid()
    love.graphics.setColor(1, 1, 1, 0.05)
    for x = 0, g_native_width, 24 do
        love.graphics.line(x, 0, x, g_native_height)
    end
    for y = 0, g_native_height, 24 do
        love.graphics.line(0, y, g_native_width, y)
    end
end

function Background:draw_wind_bands()
    local accent = self.accent_color
    for index = 1, 5 do
        local offset = ((self.time * (16 + index * 4)) + index * 42) % (g_native_width + 80)
        local x = -80 + offset
        local y = 42 + index * 28
        love.graphics.setColor(accent[1], accent[2] * 0.8, accent[3] * 1.05, 0.1)
        love.graphics.polygon("fill", x, y, x + 54, y - 12, x + 92, y + 2, x + 40, y + 16)
    end
end

function Background:draw_depth_bands()
    local base = mix_color(self.floor_color, self.color, 0.35)
    for index = 1, 3 do
        local height = 18 + index * 10
        local y = g_native_height - 80 + index * 16
        local alpha = 0.18 - index * 0.03
        love.graphics.setColor(base[1], base[2], base[3], alpha)
        love.graphics.rectangle("fill", 0, y, g_native_width, height)
    end
end

function Background:draw()
    self:draw_gradient()

    if self.scene and self.scene.mode == "proving_ground" then
        Background.draw_grid()
    elseif self.scene and self.scene.title == "LUST" then
        self:draw_wind_bands()
    else
        self:draw_arches()
    end

    self:draw_depth_bands()

    love.graphics.setColor(self.floor_color[1], self.floor_color[2], self.floor_color[3], 0.92)
    love.graphics.rectangle(
        "fill",
        0,
        math.floor(g_native_height * 0.72),
        g_native_width,
        g_native_height * 0.28
    )

    love.graphics.setColor(1, 1, 1, 1)
end

return Background
