local PlayerSpriteBank = require("player_sprite_bank")

local PreviewPlayer = {}
PreviewPlayer.__index = PreviewPlayer

local PANEL_MARGIN = 18
local HITBOX_WIDTH = 28
local HITBOX_HEIGHT = 28
local DEFAULT_ZOOM = 10

local function draw_centered_text(text, x, y, width)
    love.graphics.printf(text, x, y, width, "center")
end

local function clamp(value, minimum, maximum)
    return math.max(minimum, math.min(maximum, value))
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
    instance.zoom = DEFAULT_ZOOM
    instance.paused = false
    instance.show_overlays = true
    instance.show_onion_skin = false
    instance.show_difference = false
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
    if self.paused then
        return
    end

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

function PreviewPlayer:get_current_frame()
    local animation = self.sprite_bank.animations[self.anim_state]
    if not animation then
        return nil, nil
    end
    return animation, animation.frames[self.anim_frame]
end

function PreviewPlayer:get_frame_by_offset(offset)
    local animation = self.sprite_bank.animations[self.anim_state]
    if not animation or #animation.frames == 0 then
        return nil
    end

    local index = ((self.anim_frame - 1 + offset) % #animation.frames) + 1
    return animation.frames[index]
end

function PreviewPlayer:get_animation_diagnostics()
    local animation = self.sprite_bank.animations[self.anim_state]
    local diagnostics = {
        max_center_drift = 0,
        max_foot_drift = 0,
        width_range = 0,
        height_range = 0,
    }

    if not animation or #animation.frames == 0 then
        return diagnostics
    end

    local min_center = nil
    local max_center = nil
    local min_foot = nil
    local max_foot = nil
    local min_width = nil
    local max_width = nil
    local min_height = nil
    local max_height = nil

    for _, frame in ipairs(animation.frames) do
        local bounds = frame.bounds
        if bounds then
            min_center = min_center and math.min(min_center, bounds.center_x) or bounds.center_x
            max_center = max_center and math.max(max_center, bounds.center_x) or bounds.center_x
            min_foot = min_foot and math.min(min_foot, bounds.foot_y) or bounds.foot_y
            max_foot = max_foot and math.max(max_foot, bounds.foot_y) or bounds.foot_y
            min_width = min_width and math.min(min_width, bounds.width) or bounds.width
            max_width = max_width and math.max(max_width, bounds.width) or bounds.width
            min_height = min_height and math.min(min_height, bounds.height) or bounds.height
            max_height = max_height and math.max(max_height, bounds.height) or bounds.height
        end
    end

    diagnostics.max_center_drift = (max_center and min_center) and (max_center - min_center) or 0
    diagnostics.max_foot_drift = (max_foot and min_foot) and (max_foot - min_foot) or 0
    diagnostics.width_range = (max_width and min_width) and (max_width - min_width) or 0
    diagnostics.height_range = (max_height and min_height) and (max_height - min_height) or 0
    return diagnostics
end

function PreviewPlayer:draw_native_preview(x, y, width, height)
    local baseline_y = math.floor(y + height * 0.78)
    local center_x = math.floor(x + width * 0.5)
    local _, frame = self:get_current_frame()
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

    if self.show_overlays and frame and frame.bounds then
        local bounds = frame.bounds
        love.graphics.setColor(0.98, 0.74, 0.16, 0.95)
        love.graphics.rectangle(
            "line",
            center_x - (self.sprite_bank.frame_size / 2 - bounds.left) * metrics.scale + metrics.draw_offset.x * metrics.scale,
            baseline_y - (metrics.feet_anchor - bounds.top) * metrics.scale + metrics.draw_offset.y * metrics.scale,
            bounds.width * metrics.scale,
            bounds.height * metrics.scale
        )
    end

    love.graphics.setColor(0.85, 0.88, 0.92, 1)
    draw_centered_text("Gameplay Scale", x, y + 8, width)
end

function PreviewPlayer:draw_zoom_preview(x, y, width, height)
    local zoom = self.zoom
    local _, frame = self:get_current_frame()
    local metrics = self:get_render_metrics()
    local frame_size = self.sprite_bank.frame_size * zoom
    local frame_x = math.floor(x + math.max(18, (width - frame_size) * 0.5))
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

    love.graphics.setColor(0.26, 0.8, 0.94, 0.4)
    local center_line_x = frame_x + (self.sprite_bank.frame_size * 0.5 * zoom)
    love.graphics.line(center_line_x, frame_y, center_line_x, frame_y + frame_size)

    if self.show_onion_skin then
        local previous = self:get_frame_by_offset(-1)
        local following = self:get_frame_by_offset(1)
        if previous then
            love.graphics.setColor(0.27, 0.92, 0.66, 0.28)
            love.graphics.draw(self.sprite_bank.image, previous.quad, frame_x, frame_y, 0, zoom, zoom)
        end
        if following then
            love.graphics.setColor(0.44, 0.68, 0.98, 0.24)
            love.graphics.draw(self.sprite_bank.image, following.quad, frame_x, frame_y, 0, zoom, zoom)
        end
    end

    if self.show_difference then
        local previous = self:get_frame_by_offset(-1)
        if previous then
            love.graphics.setColor(0.94, 0.29, 0.29, 0.34)
            love.graphics.draw(self.sprite_bank.image, previous.quad, frame_x, frame_y, 0, zoom, zoom)
        end
    end

    if frame then
        if self.show_difference then
            love.graphics.setColor(0.29, 0.87, 0.95, 0.72)
        else
            love.graphics.setColor(1, 1, 1, 1)
        end
        love.graphics.draw(self.sprite_bank.image, frame.quad, frame_x, frame_y, 0, zoom, zoom)
    end

    if self.show_overlays and frame and frame.bounds then
        local bounds = frame.bounds
        love.graphics.setColor(0.98, 0.74, 0.16, 0.95)
        love.graphics.rectangle(
            "line",
            frame_x + bounds.left * zoom,
            frame_y + bounds.top * zoom,
            bounds.width * zoom,
            bounds.height * zoom
        )

        love.graphics.setColor(0.88, 0.33, 0.33, 0.85)
        local bbox_center_x = frame_x + bounds.center_x * zoom
        love.graphics.line(bbox_center_x, frame_y, bbox_center_x, frame_y + frame_size)
    end

    love.graphics.setColor(0.85, 0.88, 0.92, 1)
    draw_centered_text("10x Frame Inspector", x, y + 8, width)
    love.graphics.print(
        string.format("Feet anchor: %d px", metrics.feet_anchor),
        x + 16,
        y + height - 96
    )
    love.graphics.print(
        string.format("Render scale: %.2fx  |  Zoom: %dx", metrics.scale, zoom),
        x + 16,
        y + height - 78
    )
    local mode_label = "Mode: normal"
    if self.show_difference then
        mode_label = "Mode: frame diff"
    elseif self.show_onion_skin then
        mode_label = "Mode: onion skin"
    end
    love.graphics.setColor(0.78, 0.82, 0.88, 1)
    love.graphics.print(mode_label, x + 16, y + height - 114)
    if frame and frame.bounds then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.print(
            string.format(
                "Alpha bounds: L%d T%d R%d B%d  |  WxH %dx%d",
                frame.bounds.left,
                frame.bounds.top,
                frame.bounds.right,
                frame.bounds.bottom,
                frame.bounds.width,
                frame.bounds.height
            ),
            x + 16,
            y + height - 60
        )
        love.graphics.setColor(0.88, 0.33, 0.33, 1)
        love.graphics.print(
            string.format(
                "Center drift: %+0.1f px  |  Foot drift: %+0.1f px",
                frame.bounds.center_x - frame.bounds.frame_center_x,
                frame.bounds.foot_y - metrics.feet_anchor
            ),
            x + 16,
            y + height - 42
        )
    end
    if metrics.placeholder then
        love.graphics.setColor(0.92, 0.55, 0.18, 1)
        love.graphics.print("Temporary placeholder art", x + 16, y + height - 24)
    end
end

function PreviewPlayer:draw_frame_strip(x, y, width, height)
    local animation = self.sprite_bank.animations[self.anim_state]
    if not animation or #animation.frames == 0 then
        return
    end

    love.graphics.setColor(0.07, 0.08, 0.1, 1)
    love.graphics.rectangle("fill", x, y, width, height)

    local thumb_scale = 2
    local thumb_size = self.sprite_bank.frame_size * thumb_scale
    local gap = 16
    local total_width = #animation.frames * thumb_size + (#animation.frames - 1) * gap
    local start_x = x + math.max(16, math.floor((width - total_width) * 0.5))
    local top_y = y + 14

    for index, frame in ipairs(animation.frames) do
        local frame_x = start_x + (index - 1) * (thumb_size + gap)
        local is_active = index == self.anim_frame
        love.graphics.setColor(is_active and 0.95 or 0.35, is_active and 0.62 or 0.39, is_active and 0.22 or 0.46, 1)
        love.graphics.rectangle("line", frame_x - 2, top_y - 2, thumb_size + 4, thumb_size + 4)
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.draw(self.sprite_bank.image, frame.quad, frame_x, top_y, 0, thumb_scale, thumb_scale)

        if self.show_overlays and frame.bounds then
            love.graphics.setColor(0.98, 0.74, 0.16, 0.9)
            love.graphics.rectangle(
                "line",
                frame_x + frame.bounds.left * thumb_scale,
                top_y + frame.bounds.top * thumb_scale,
                frame.bounds.width * thumb_scale,
                frame.bounds.height * thumb_scale
            )
        end

        love.graphics.setColor(0.85, 0.88, 0.92, 1)
        love.graphics.printf(
            string.format("%d", index),
            frame_x,
            top_y + thumb_size + 6,
            thumb_size,
            "center"
        )
    end
end

function PreviewPlayer:draw()
    local width, height = love.graphics.getDimensions()
    local left_width = math.floor(width * 0.33)
    local strip_height = 180
    local panel_height = height - 110 - strip_height
    local right_x = PANEL_MARGIN + left_width + PANEL_MARGIN
    local right_width = width - right_x - PANEL_MARGIN
    local strip_y = 86 + panel_height + PANEL_MARGIN

    love.graphics.clear(0.05, 0.06, 0.08, 1)
    self:draw_native_preview(PANEL_MARGIN, 86, left_width, panel_height)
    self:draw_zoom_preview(right_x, 86, right_width, panel_height)
    self:draw_frame_strip(PANEL_MARGIN, strip_y, width - PANEL_MARGIN * 2, strip_height)

    local diagnostics = self:get_animation_diagnostics()
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.print("Player Animation Audit", PANEL_MARGIN, 22)
    love.graphics.print("Left/Right: change animation  |  Up/Down: frame  |  +/-: zoom  |  Space: pause", PANEL_MARGIN, 44)
    love.graphics.print("B: bounds  |  O: onion skin  |  F: frame diff  |  Esc: quit preview", PANEL_MARGIN, 62)
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
    love.graphics.print(
        string.format(
            "Anim drift: foot %.1f px  |  center %.1f px  |  height range %d px",
            diagnostics.max_foot_drift,
            diagnostics.max_center_drift,
            diagnostics.height_range
        ),
        right_x,
        62
    )
end

function PreviewPlayer:keypressed(key)
    if key == "right" or key == "d" then
        self:cycle_animation(1)
    elseif key == "left" or key == "a" then
        self:cycle_animation(-1)
    elseif key == "up" or key == "w" then
        local frame_count = #self.sprite_bank.animations[self.anim_state].frames
        self.anim_frame = ((self.anim_frame - 2) % frame_count) + 1
        self.anim_timer = 0
    elseif key == "down" or key == "s" then
        local frame_count = #self.sprite_bank.animations[self.anim_state].frames
        self.anim_frame = (self.anim_frame % frame_count) + 1
        self.anim_timer = 0
    elseif key == "=" or key == "+" then
        self.zoom = clamp(self.zoom + 1, 2, 12)
    elseif key == "-" then
        self.zoom = clamp(self.zoom - 1, 2, 12)
    elseif key == "space" then
        self.paused = not self.paused
    elseif key == "b" then
        self.show_overlays = not self.show_overlays
    elseif key == "o" then
        self.show_onion_skin = not self.show_onion_skin
        if self.show_onion_skin then
            self.show_difference = false
        end
    elseif key == "f" then
        self.show_difference = not self.show_difference
        if self.show_difference then
            self.show_onion_skin = false
        end
    elseif key == "escape" then
        love.event.quit()
    end
end

return PreviewPlayer
