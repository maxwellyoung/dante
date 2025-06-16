local Animation = {}
Animation.__index = Animation

function Animation.new(image_path, frame_width, frame_height, duration, frames)
    local self = setmetatable({}, Animation)
    self.image = love.graphics.newImage(image_path)
    self.quads = {}
    self.duration = duration or 1
    self.timer = 0
    self.current_frame_index = 1
    
    local all_quads = {}
    local img_width = self.image:getWidth()
    local img_height = self.image:getHeight()

    -- Read quads left-to-right, then top-to-bottom
    for y = 0, img_height - frame_height, frame_height do
        for x = 0, img_width - frame_width, frame_width do
            table.insert(all_quads, love.graphics.newQuad(x, y, frame_width, frame_height, img_width, img_height))
        end
    end
    
    if frames then
        for _, frame_num in ipairs(frames) do
            if all_quads[frame_num] then
                table.insert(self.quads, all_quads[frame_num])
            end
        end
    else
        self.quads = all_quads
    end
    
    return self
end

function Animation:update(dt)
    if #self.quads > 1 and self.duration > 0 then
        self.timer = self.timer + dt
        local frame_duration = self.duration / #self.quads
        while self.timer >= frame_duration do
            self.timer = self.timer - frame_duration
            self.current_frame_index = (self.current_frame_index % #self.quads) + 1
        end
    end
end

function Animation:draw(x, y, angle, sx, sy, ox, oy)
    if self.quads[self.current_frame_index] then
        love.graphics.draw(self.image, self.quads[self.current_frame_index], x, y, angle, sx, sy, ox, oy)
    end
end

return Animation