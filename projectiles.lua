-- projectiles.lua
-- Manages player-fired tracers with immediate hit and ricochet resolution.

local Utils = require("utils")

local Projectiles = {}
Projectiles.__index = Projectiles

local MAX_RANGE = 560
local STEP_SIZE = 8
local BOUNCE_OFFSET = 4
local DEFAULT_RICOCHETS = 1

local function is_solid(level, x, y)
    return level:get_tile(x, y) == 1
end

local function resolve_bounce_axis(level, previous_x, previous_y, sample_x, sample_y, dir_x, dir_y)
    local blocked_x = is_solid(level, sample_x, previous_y)
    local blocked_y = is_solid(level, previous_x, sample_y)

    if blocked_x and not blocked_y then
        return "vertical"
    end
    if blocked_y and not blocked_x then
        return "horizontal"
    end
    if math.abs(dir_x) >= math.abs(dir_y) then
        return "vertical"
    end
    return "horizontal"
end

function Projectiles:new(services)
    local class = self or Projectiles
    local instance = setmetatable({}, class)
    instance.services = services
    instance.items = {}
    return instance
end

function Projectiles:add_tracer(points, hit_kind, ricochet_count)
    local color = { 1, 0.88, 0.45, 1 }
    local line_width = 2
    if hit_kind == "wall" then
        color = { 1, 0.72, 0.42, 1 }
    elseif hit_kind == "enemy" then
        color = { 1, 0.94, 0.72, 1 }
    elseif ricochet_count > 0 then
        color = { 1, 0.84, 0.58, 1 }
        line_width = 3
    end

    table.insert(self.items, {
        points = points,
        lifetime = ricochet_count > 0 and 0.14 or 0.08,
        max_lifetime = ricochet_count > 0 and 0.14 or 0.08,
        color = color,
        line_width = line_width,
    })
end

function Projectiles:spawn(x, y, target_x, target_y, weapon_profile)
    local level = self.services and self.services.level or g_level
    local sfx = self.services and self.services.sfx or g_sfx
    local effects = self.services and self.services.effects or g_effects
    local camera = self.services and self.services.camera or g_camera
    local enemies = self.services and self.services.enemies or g_enemies
    local run_stats = self.services and self.services.run_stats or g_run_stats
    local profile = weapon_profile or {}
    local ricochet_budget = profile.ricochets
    if ricochet_budget == nil then
        ricochet_budget = DEFAULT_RICOCHETS
    end

    local dx = target_x - x
    local dy = target_y - y
    local distance = math.max(1, math.sqrt(dx * dx + dy * dy))
    local dir_x = dx / distance
    local dir_y = dy / distance
    local origin_x = x
    local origin_y = y
    local remaining_range = profile.range or MAX_RANGE
    local ricochet_count = 0
    local hit_kind = "miss"
    local points = {
        { x = x, y = y },
    }

    while remaining_range > 0 do
        local previous_x = origin_x
        local previous_y = origin_y
        local segment_hit = false

        for travel = STEP_SIZE, remaining_range, STEP_SIZE do
            local sample_x = origin_x + dir_x * travel
            local sample_y = origin_y + dir_y * travel
            local probe = {
                x = sample_x - 2,
                y = sample_y - 2,
                width = 4,
                height = 4,
            }

            for enemy_index = #enemies.items, 1, -1 do
                local enemy = enemies.items[enemy_index]
                if Utils.check_collision(probe, enemy) then
                    hit_kind = "enemy"
                    points[#points + 1] = { x = sample_x, y = sample_y }
                    sfx:play("hit_enemy")
                    effects:spawn("blood", sample_x, sample_y)
                    camera:shake(5, 0.12)
                    if run_stats then
                        run_stats.shots_hit = run_stats.shots_hit + 1
                        run_stats.room_shots_hit = run_stats.room_shots_hit + 1
                        if ricochet_count > 0 then
                            run_stats.shot_bank_kills = (run_stats.shot_bank_kills or 0) + 1
                            run_stats.room_shot_bank_kills = (run_stats.room_shot_bank_kills or 0) + 1
                            if effects then
                                effects:spawn("ricochet_kill", sample_x, sample_y)
                            end
                            sfx:play("ricochet_kill")
                            if camera then
                                camera:shake(8, 0.18)
                            end
                            local trigger_hitstop = self.services and self.services.trigger_hitstop or g_trigger_hitstop
                            if trigger_hitstop then
                                trigger_hitstop(0.08)
                            end
                        end
                    end
                    enemy:destroy()
                    table.remove(enemies.items, enemy_index)
                    self:add_tracer(points, hit_kind, ricochet_count)
                    sfx:play("shoot")
                    return
                end
            end

            if is_solid(level, sample_x, sample_y) then
                local hit_x = previous_x
                local hit_y = previous_y
                points[#points + 1] = { x = hit_x, y = hit_y }
                effects:spawn(ricochet_count < ricochet_budget and "ricochet_sparks" or "sparks", hit_x, hit_y)
                sfx:play("hit_wall")

                if ricochet_count < ricochet_budget then
                    local axis = resolve_bounce_axis(level, previous_x, previous_y, sample_x, sample_y, dir_x, dir_y)
                    if axis == "vertical" then
                        dir_x = -dir_x
                    else
                        dir_y = -dir_y
                    end
                    ricochet_count = ricochet_count + 1
                    camera:shake(3, 0.07)
                    if run_stats then
                        run_stats.shot_ricochets = (run_stats.shot_ricochets or 0) + 1
                        run_stats.room_shot_ricochets = (run_stats.room_shot_ricochets or 0) + 1
                    end
                    remaining_range = remaining_range - travel
                    origin_x = hit_x + dir_x * BOUNCE_OFFSET
                    origin_y = hit_y + dir_y * BOUNCE_OFFSET
                    points[#points + 1] = { x = origin_x, y = origin_y }
                    segment_hit = true
                    break
                end

                hit_kind = "wall"
                self:add_tracer(points, hit_kind, ricochet_count)
                sfx:play("shoot")
                return
            end

            previous_x = sample_x
            previous_y = sample_y
        end

        if segment_hit then
            if remaining_range <= STEP_SIZE then
                break
            end
        else
            points[#points + 1] = {
                x = origin_x + dir_x * remaining_range,
                y = origin_y + dir_y * remaining_range,
            }
            break
        end
    end

    self:add_tracer(points, hit_kind, ricochet_count)
    sfx:play("shoot")
end

function Projectiles:update(dt, _level, _enemies)
    for index = #self.items, 1, -1 do
        local projectile = self.items[index]
        projectile.lifetime = projectile.lifetime - dt
        if projectile.lifetime <= 0 then
            table.remove(self.items, index)
        end
    end
end

function Projectiles:draw()
    for _, projectile in ipairs(self.items) do
        local alpha = math.max(0, projectile.lifetime / projectile.max_lifetime)
        love.graphics.setColor(projectile.color[1], projectile.color[2], projectile.color[3], alpha)
        love.graphics.setLineWidth(projectile.line_width or 2)
        for index = 1, #projectile.points - 1 do
            local start_point = projectile.points[index]
            local end_point = projectile.points[index + 1]
            love.graphics.line(start_point.x, start_point.y, end_point.x, end_point.y)
        end
        local end_point = projectile.points[#projectile.points]
        love.graphics.setColor(1, 1, 1, alpha * 0.9)
        love.graphics.circle("fill", end_point.x, end_point.y, 2)
    end
    love.graphics.setLineWidth(1)
    love.graphics.setColor(1, 1, 1, 1)
end

return Projectiles
