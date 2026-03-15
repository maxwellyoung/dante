local PlayerSpriteBank = {}

local function compute_alpha_bounds(path, frame_size)
    if not path or not love.filesystem.getInfo(path) then
        return nil
    end

    local image_data = love.image.newImageData(path)
    local width, height = image_data:getDimensions()
    local min_x, min_y = width, height
    local max_x, max_y = -1, -1

    for y = 0, height - 1 do
        for x = 0, width - 1 do
            local _, _, _, alpha = image_data:getPixel(x, y)
            if alpha > 0 then
                if x < min_x then
                    min_x = x
                end
                if y < min_y then
                    min_y = y
                end
                if x > max_x then
                    max_x = x
                end
                if y > max_y then
                    max_y = y
                end
            end
        end
    end

    if max_x < min_x or max_y < min_y then
        return nil
    end

    return {
        left = min_x,
        top = min_y,
        right = max_x + 1,
        bottom = max_y + 1,
        width = (max_x - min_x) + 1,
        height = (max_y - min_y) + 1,
        center_x = (min_x + max_x + 1) / 2,
        center_y = (min_y + max_y + 1) / 2,
        foot_y = max_y + 1,
        frame_center_x = frame_size / 2,
    }
end

local function copy_point(point)
    return {
        x = point and point.x or 0,
        y = point and point.y or 0,
    }
end

local function merge_render(defaults, override)
    local merged = {
        scale = defaults.scale or 1,
        feet_anchor = defaults.feet_anchor,
        draw_offset = copy_point(defaults.draw_offset),
        placeholder = defaults.placeholder == true,
    }

    if override then
        if override.scale ~= nil then
            merged.scale = override.scale
        end
        if override.feet_anchor ~= nil then
            merged.feet_anchor = override.feet_anchor
        end
        if override.draw_offset ~= nil then
            merged.draw_offset = copy_point(override.draw_offset)
        end
        if override.placeholder ~= nil then
            merged.placeholder = override.placeholder == true
        end
    end

    return merged
end

local function load_manifest(path)
    local chunk, err = love.filesystem.load(path)
    assert(
        chunk,
        string.format("Failed to load player manifest '%s': %s", path, err or "unknown error")
    )
    return chunk()
end

local function build_animation(sw, sh, animation, default_render, frame_size)
    local frames = {}
    for _, frame in ipairs(animation.frames or {}) do
        local bounds = compute_alpha_bounds(frame.source, frame_size)
        frames[#frames + 1] = {
            quad = love.graphics.newQuad(frame.x, frame.y, frame.w, frame.h, sw, sh),
            source = frame.source,
            bounds = bounds,
        }
    end

    return {
        fps = animation.fps or 8,
        loop = animation.loop ~= false,
        lock_frame_01 = animation.lock_frame_01 == true,
        placeholder = animation.placeholder == true,
        render = merge_render(default_render, animation.render),
        frames = frames,
    }
end

function PlayerSpriteBank.load(path)
    local manifest = load_manifest(path)
    local image = love.graphics.newImage(manifest.atlas_path)
    image:setFilter("nearest", "nearest")

    local sw, sh = image:getDimensions()
    local bank = {
        image = image,
        frame_size = manifest.frame_size,
        render = merge_render({
            scale = 1,
            feet_anchor = manifest.frame_size,
            draw_offset = { x = 0, y = 0 },
            placeholder = false,
        }, manifest.render),
        animation_order = manifest.animation_order or {},
        animations = {},
    }

    for _, name in ipairs(bank.animation_order) do
        bank.animations[name] =
            build_animation(sw, sh, manifest.animations[name] or {}, bank.render, bank.frame_size)
    end

    for name, animation in pairs(manifest.animations or {}) do
        if bank.animations[name] == nil then
            bank.animations[name] = build_animation(sw, sh, animation, bank.render, bank.frame_size)
        end
    end

    return bank
end

return PlayerSpriteBank
