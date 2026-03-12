local PlayerSpriteBank = require("player_sprite_bank")

local PreviewPlayer = {}
PreviewPlayer.__index = PreviewPlayer

local PANEL_MARGIN = 18
local HITBOX_WIDTH = 28
local HITBOX_HEIGHT = 28

local function draw_centered_text(text, x, y, width)
    love.graphics.printf(text, x, y, width, "center")
end

function PreviewPlayer:new()
    local class = self or PreviewPlayer
    local instance = setmetatable({}, class)
    instance.sprite_bank = nil
    instance.animation_order = {}
    instance.animation_index = 1
    instance.anim_state = "idle"
    instance.anim_frame = 1
    instance.anim_timer = 0
    return instance
end

function PreviewPlayer:load()
    self.sprite_bank = PlayerSpriteBank.load("assets/player/player_manifest.lua")
    self.animation_order = self.sprite_bank.animation_order
    self:set_animation(self.animation_order[1] or "idle")
end

function PreviewPlayer:set_animation(name)
    if not self.sprite_bank.animations[name] then
        return
    end

    self.anim_state = name
    self.anim_frame = 1
    self.anim_timer = 0
end

function PreviewPlayer:cycle_animation(delta)
    if #self.animation_order == 0 then
        return
    end

    self.animation_index = ((self.animation_index - 1 + delta) % #self.animation_order) + 1
    self:set_animation(self.animation_order[self.animation_index])
end

function PreviewPlayer:update(dt)
    local animation = self.sprite_bank.animations[self.anim_state]
    if not animation or #animation.frames == 0 then
        return
    end

    local frame_duration = 1 / math.max(animation.fps, 1)
    self.anim_timer = self.anim_timer + dt

    while self.anim_timer >= frame_duration do
        self.anim_timer = self.anim_timer - frame_duration
        self.anim_frame = self.anim_frame + 1
        if self.anim_frame > #animation.frames then
            self.anim_frame = 1
        end
    end
end

function PreviewPlayer:get_render_metrics()
    local animation = self.sprite_bank.animations[self.anim_state]
    local render = animation.render
    return {
        scale = render.scale or 1,
        feet_anchor = render.feet_anchor or self.sprite_bank.frame_size,
        draw_offset = render.draw_offset or { x = 0, y = 0 },
        placeholder = render.placeholder == true or animation.placeholder == true,
    }
end

function PreviewPlayer:draw_native_preview(x, y, width, height)
    local baseline_y = math.floor(y + height * 0.78)
    local center_x = math.floor(x + width * 0.5)
    local animation = self.sprite_bank.animations[self.anim_state]
    local frame = animation and animation.frames[self.anim_frame]
    local metrics = self:get_render_metrics()

    love.graphics.setColor(0.09, 0.1, 0.13, 1)
    love.graphics.rectangle("fill", x, y, width, height)
    love.graphics.setColor(0.18, 0.2, 0.24, 1)
    love.graphics.rectangle("fill", x, baseline_y, width, height - (baseline_y - y))
    love.graphics.setColor(0.35, 0.39, 0.46, 1)
    love.graphics.line(x, baseline_y, x + width, baseline_y)

    love.graphics.setColor(0.25, 0.75, 0.95, 0.35)
    love.graphics.rectangle(
        "line",
        center_x - (HITBOX_WIDTH / 2),
        baseline_y - HITBOX_HEIGHT,
        HITBOX_WIDTH,
        HITBOX_HEIGHT
    )

    if frame then
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.draw(
            self.sprite_bank.image,
            frame.quad,
            center_x + metrics.draw_offset.x * metrics.scale,
            baseline_y + metrics.draw_offset.y * metrics.scale,
            0,
            metrics.scale,
            metrics.scale,
            self.sprite_bank.frame_size / 2,
            metrics.feet_anchor
        )
    end

    love.graphics.setColor(0.85, 0.88, 0.92, 1)
    draw_centered_text("Gameplay Scale", x, y + 8, width)
end

function PreviewPlayer:draw_zoom_preview(x, y, width, height)
    local zoom = 4
    local animation = self.sprite_bank.animations[self.anim_state]
    local frame = animation and animation.frames[self.anim_frame]
    local metrics = self:get_render_metrics()
    local frame_size = self.sprite_bank.frame_size * zoom
    local frame_x = math.floor(x + (width - frame_size) * 0.5)
    local frame_y = math.floor(y + 30)

    love.graphics.setColor(0.07, 0.08, 0.1, 1)
    love.graphics.rectangle("fill", x, y, width, height)

    love.graphics.setColor(0.16, 0.17, 0.2, 1)
    for column = 0, self.sprite_bank.frame_size do
        local gx = frame_x + column * zoom
        love.graphics.line(gx, frame_y, gx, frame_y + frame_size)
    end
    for row = 0, self.sprite_bank.frame_size do
        local gy = frame_y + row * zoom
        love.graphics.line(frame_x, gy, frame_x + frame_size, gy)
    end

    love.graphics.setColor(0.35, 0.39, 0.46, 1)
    love.graphics.rectangle("line", frame_x, frame_y, frame_size, frame_size)

    love.graphics.setColor(0.92, 0.55, 0.18, 0.65)
    local feet_y = frame_y + metrics.feet_anchor * zoom
    love.graphics.line(frame_x, feet_y, frame_x + frame_size, feet_y)

    if frame then
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.draw(self.sprite_bank.image, frame.quad, frame_x, frame_y, 0, zoom, zoom)
    end

    love.graphics.setColor(0.85, 0.88, 0.92, 1)
    draw_centered_text("Frame Inspector", x, y + 8, width)
    love.graphics.print(
        string.format("Feet anchor: %d px", metrics.feet_anchor),
        x + 16,
        y + height - 62
    )
    love.graphics.print(
        string.format("Render scale: %.2fx", metrics.scale),
        x + 16,
        y + height - 44
    )
    if metrics.placeholder then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.print("Temporary placeholder art", x + 16, y + height - 24)
    end
end

function PreviewPlayer:draw()
    local width, height = love.graphics.getDimensions()
    local left_width = math.floor(width * 0.46)
    local panel_height = height - 150
    local right_x = PANEL_MARGIN + left_width + PANEL_MARGIN
    local right_width = width - right_x - PANEL_MARGIN

    love.graphics.clear(0.05, 0.06, 0.08, 1)
    self:draw_native_preview(PANEL_MARGIN, 86, left_width, panel_height)
    self:draw_zoom_preview(right_x, 86, right_width, panel_height)

    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.print("Player Scale Inspector", PANEL_MARGIN, 22)
    love.graphics.print("Left/Right: change animation", PANEL_MARGIN, 44)
    love.graphics.print("Esc: quit preview", PANEL_MARGIN, 62)
    love.graphics.print(string.format("Animation: %s", self.anim_state), right_x, 22)
    love.graphics.print(
        string.format(
            "Frame: %d/%d",
            self.anim_frame,
            #self.sprite_bank.animations[self.anim_state].frames
        ),
        right_x,
        44
    )
end

function PreviewPlayer:keypressed(key)
    if key == "right" or key == "d" then
        self:cycle_animation(1)
    elseif key == "left" or key == "a" then
        self:cycle_animation(-1)
    elseif key == "escape" then
        love.event.quit()
    end
end

return PreviewPlayer
