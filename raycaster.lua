-- raycaster.lua
-- Lo-fi raycasting FPS renderer. Wolfenstein 3D style.
-- PS1 aesthetic: low resolution, nearest-neighbor, limited draw distance.

local Raycaster = {}
Raycaster.__index = Raycaster

local TILE_SIZE = 1  -- world units per tile
local FOV = math.pi / 3  -- 60 degrees
local WALL_HEIGHT = 1

function Raycaster:new(width, height)
    local instance = setmetatable({}, self)
    instance.screen_width = width
    instance.screen_height = height
    instance.half_height = height / 2
    instance.max_depth = 16
    instance.fog_start = 6
    instance.fog_color = { 0.02, 0.02, 0.04 }
    instance.wall_colors = {
        [1] = { 0.45, 0.48, 0.55 },  -- stone
        [2] = { 0.65, 0.25, 0.2 },   -- hell stone
        [3] = { 0.3, 0.5, 0.25 },    -- sludge wall
    }
    instance.floor_color = { 0.12, 0.13, 0.16 }
    instance.ceiling_color = { 0.06, 0.06, 0.08 }
    instance.z_buffer = {}
    return instance
end

function Raycaster:cast_walls(map, px, py, angle)
    local rays = {}
    local step = FOV / self.screen_width

    for col = 0, self.screen_width - 1 do
        local ray_angle = angle - FOV / 2 + col * step
        local sin_a = math.sin(ray_angle)
        local cos_a = math.cos(ray_angle)

        local depth = 0
        local hit_wall = 0
        local hit_side = 0  -- 0 = vertical, 1 = horizontal (for shading)
        local hit_x = 0

        -- DDA raycasting
        local map_x = math.floor(px)
        local map_y = math.floor(py)

        local delta_x = math.abs(cos_a) > 0.0001 and math.abs(1 / cos_a) or 1e10
        local delta_y = math.abs(sin_a) > 0.0001 and math.abs(1 / sin_a) or 1e10

        local step_x, step_y
        local side_x, side_y

        if cos_a < 0 then
            step_x = -1
            side_x = (px - map_x) * delta_x
        else
            step_x = 1
            side_x = (map_x + 1 - px) * delta_x
        end

        if sin_a < 0 then
            step_y = -1
            side_y = (py - map_y) * delta_y
        else
            step_y = 1
            side_y = (map_y + 1 - py) * delta_y
        end

        -- Step through grid
        for _ = 1, 64 do
            if side_x < side_y then
                side_x = side_x + delta_x
                map_x = map_x + step_x
                hit_side = 0
            else
                side_y = side_y + delta_y
                map_y = map_y + step_y
                hit_side = 1
            end

            local tile = map:get(map_x, map_y)
            if tile and tile > 0 then
                hit_wall = tile
                -- Calculate perpendicular distance (fish-eye correction)
                if hit_side == 0 then
                    depth = side_x - delta_x
                    hit_x = py + depth * sin_a
                else
                    depth = side_y - delta_y
                    hit_x = px + depth * cos_a
                end
                hit_x = hit_x - math.floor(hit_x)  -- fractional part for texturing
                break
            end
        end

        -- Correct fish-eye
        depth = depth * math.cos(ray_angle - angle)

        rays[col] = {
            depth = depth,
            wall = hit_wall,
            side = hit_side,
            hit_x = hit_x,
        }
        self.z_buffer[col] = depth
    end

    return rays
end

function Raycaster:draw_scene(rays)
    -- Draw ceiling
    love.graphics.setColor(self.ceiling_color[1], self.ceiling_color[2], self.ceiling_color[3], 1)
    love.graphics.rectangle("fill", 0, 0, self.screen_width, self.half_height)

    -- Draw floor
    love.graphics.setColor(self.floor_color[1], self.floor_color[2], self.floor_color[3], 1)
    love.graphics.rectangle("fill", 0, self.half_height, self.screen_width, self.half_height)

    -- Draw walls column by column
    for col = 0, self.screen_width - 1 do
        local ray = rays[col]
        if ray and ray.depth > 0 and ray.wall > 0 then
            local wall_height = (WALL_HEIGHT / ray.depth) * self.screen_height * 0.6
            local wall_top = self.half_height - wall_height / 2
            local wall_bottom = self.half_height + wall_height / 2

            -- Base wall color
            local base = self.wall_colors[ray.wall] or self.wall_colors[1]
            local shade = ray.side == 1 and 0.7 or 1.0

            -- Distance fog
            local fog = 0
            if ray.depth > self.fog_start then
                fog = math.min(1, (ray.depth - self.fog_start) / (self.max_depth - self.fog_start))
            end

            local r = base[1] * shade * (1 - fog) + self.fog_color[1] * fog
            local g = base[2] * shade * (1 - fog) + self.fog_color[2] * fog
            local b = base[3] * shade * (1 - fog) + self.fog_color[3] * fog

            -- Simple vertical stripe texture
            local stripe_shade = 1
            if ray.hit_x < 0.05 or ray.hit_x > 0.95 then
                stripe_shade = 0.8  -- mortar lines
            end

            love.graphics.setColor(r * stripe_shade, g * stripe_shade, b * stripe_shade, 1)
            love.graphics.rectangle("fill", col, wall_top, 1, wall_bottom - wall_top)

            -- Top/bottom edge highlight
            love.graphics.setColor(r * 1.2, g * 1.2, b * 1.2, 0.3)
            love.graphics.rectangle("fill", col, wall_top, 1, 1)
            love.graphics.setColor(r * 0.5, g * 0.5, b * 0.5, 0.3)
            love.graphics.rectangle("fill", col, wall_bottom - 1, 1, 1)

            -- Floor shading (gradient from wall base to bottom)
            for floor_y = math.max(0, math.floor(wall_bottom)), self.screen_height - 1, 3 do
                local floor_dist = (floor_y - self.half_height) / self.half_height
                local floor_fog = math.min(1, floor_dist * 0.8)
                love.graphics.setColor(
                    self.floor_color[1] * (1 - floor_fog * 0.5),
                    self.floor_color[2] * (1 - floor_fog * 0.5),
                    self.floor_color[3] * (1 - floor_fog * 0.5),
                    1
                )
                love.graphics.rectangle("fill", col, floor_y, 1, 3)
            end
        end
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function Raycaster:draw_sprites(sprites, px, py, angle)
    -- Sort sprites by distance (far to near)
    local visible = {}
    for _, sprite in ipairs(sprites) do
        if sprite.active ~= false then
            local dx = sprite.x - px
            local dy = sprite.y - py
            local dist = math.sqrt(dx * dx + dy * dy)
            if dist < self.max_depth and dist > 0.2 then
                local sprite_angle = math.atan2(dy, dx) - angle
                -- Normalize angle
                while sprite_angle > math.pi do sprite_angle = sprite_angle - math.pi * 2 end
                while sprite_angle < -math.pi do sprite_angle = sprite_angle + math.pi * 2 end

                if math.abs(sprite_angle) < FOV / 2 + 0.2 then
                    table.insert(visible, {
                        sprite = sprite,
                        dist = dist,
                        angle = sprite_angle,
                    })
                end
            end
        end
    end

    table.sort(visible, function(a, b) return a.dist > b.dist end)

    for _, entry in ipairs(visible) do
        local s = entry.sprite
        local dist = entry.dist

        -- Screen position
        local screen_x = (entry.angle / FOV + 0.5) * self.screen_width
        local sprite_height = (1 / dist) * self.screen_height * 0.5
        local sprite_width = sprite_height * (s.width_ratio or 1)
        local sprite_top = self.half_height - sprite_height / 2

        -- Distance fog
        local fog = 0
        if dist > self.fog_start then
            fog = math.min(0.8, (dist - self.fog_start) / (self.max_depth - self.fog_start))
        end

        local color = s.color or { 1, 0.3, 0.2 }
        local r = color[1] * (1 - fog)
        local g = color[2] * (1 - fog)
        local b = color[3] * (1 - fog)

        -- Draw billboard as colored rectangle (lo-fi style)
        local start_col = math.floor(screen_x - sprite_width / 2)
        local end_col = math.floor(screen_x + sprite_width / 2)

        for col = math.max(0, start_col), math.min(self.screen_width - 1, end_col) do
            -- Z-buffer check
            if self.z_buffer[col] and dist < self.z_buffer[col] then
                love.graphics.setColor(r, g, b, 1)
                love.graphics.rectangle("fill", col, sprite_top, 1, sprite_height)

                -- Simple face detail (eyes)
                if s.has_face and sprite_height > 12 then
                    local face_y = sprite_top + sprite_height * 0.3
                    local col_ratio = (col - start_col) / math.max(1, end_col - start_col)
                    if (col_ratio > 0.3 and col_ratio < 0.4) or (col_ratio > 0.6 and col_ratio < 0.7) then
                        love.graphics.setColor(1, 1, 0.8, 1 - fog)
                        love.graphics.rectangle("fill", col, face_y, 1, math.max(2, sprite_height * 0.08))
                    end
                end
            end
        end
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function Raycaster:draw_weapon(weapon_state, time)
    local w = self.screen_width
    local h = self.screen_height

    -- Weapon bob
    local bob_x = math.sin(time * 6) * 3
    local bob_y = math.abs(math.sin(time * 6)) * 2

    -- Gun position
    local gun_x = w / 2 - 16 + bob_x
    local gun_y = h - 40 + bob_y

    -- Recoil
    if weapon_state == "firing" then
        gun_y = gun_y - 6
    end

    -- Gun body (lo-fi rectangles)
    love.graphics.setColor(0.3, 0.3, 0.35, 1)
    love.graphics.rectangle("fill", gun_x, gun_y, 32, 24, 2, 2)
    love.graphics.setColor(0.4, 0.4, 0.45, 1)
    love.graphics.rectangle("fill", gun_x + 12, gun_y - 8, 8, 12)
    -- Barrel
    love.graphics.setColor(0.25, 0.25, 0.3, 1)
    love.graphics.rectangle("fill", gun_x + 13, gun_y - 16, 6, 10)

    -- Muzzle flash
    if weapon_state == "firing" then
        love.graphics.setColor(1, 0.9, 0.4, 0.9)
        love.graphics.circle("fill", gun_x + 16, gun_y - 18, 6)
        love.graphics.setColor(1, 1, 0.8, 0.5)
        love.graphics.circle("fill", gun_x + 16, gun_y - 18, 10)
    end

    love.graphics.setColor(1, 1, 1, 1)
end

function Raycaster:draw_world_text(texts, px, py, angle)
    -- Render text floating in 3D space as billboards
    for _, t in ipairs(texts) do
        if t.visible ~= false then
            local dx = t.x - px
            local dy = t.y - py
            local dist = math.sqrt(dx * dx + dy * dy)

            if dist < self.max_depth and dist > 0.3 then
                local text_angle = math.atan2(dy, dx) - angle
                while text_angle > math.pi do text_angle = text_angle - math.pi * 2 end
                while text_angle < -math.pi do text_angle = text_angle + math.pi * 2 end

                if math.abs(text_angle) < FOV / 2 + 0.1 then
                    local screen_x = (text_angle / FOV + 0.5) * self.screen_width
                    local height_offset = (t.height or 0.5) / dist * self.screen_height * 0.6
                    local screen_y = self.half_height - height_offset

                    -- Fade based on distance
                    local alpha = math.max(0, 1 - dist / self.max_depth)
                    -- Fade based on proximity (too close = fade out)
                    if dist < 1 then
                        alpha = alpha * dist
                    end

                    local color = t.color or { 0.85, 0.88, 0.95 }
                    love.graphics.setColor(color[1], color[2], color[3], alpha * (t.alpha or 1))

                    local font_scale = math.max(0.5, 2 / dist)
                    local text_width = 200
                    love.graphics.printf(
                        t.text,
                        screen_x - text_width / 2,
                        screen_y,
                        text_width,
                        "center"
                    )
                end
            end
        end
    end
    love.graphics.setColor(1, 1, 1, 1)
end

function Raycaster:draw_crosshair()
    local cx = self.screen_width / 2
    local cy = self.screen_height / 2
    love.graphics.setColor(1, 1, 1, 0.6)
    love.graphics.line(cx - 4, cy, cx - 1, cy)
    love.graphics.line(cx + 1, cy, cx + 4, cy)
    love.graphics.line(cx, cy - 4, cx, cy - 1)
    love.graphics.line(cx, cy + 1, cx, cy + 4)
    love.graphics.setColor(1, 1, 1, 1)
end

return Raycaster
