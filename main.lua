local Player = require("player")
local Level = require("level")

function love.load()
    -- This function is called exactly once at the beginning of the game.
    print("Infernal Ascent is loading!")
    player = Player:new(400, 500)
    level = Level:new()
end

function love.update(dt)
    -- This function is called every frame. 'dt' is the time since the last frame.
    player:update(dt)
    level:handle_collisions(player)
end

function love.draw()
    -- This function is called every frame to draw things to the screen.
    level:draw()
    player:draw()
end

function love.keypressed(key)
    if key == "escape" then
        love.event.quit()
    elseif key == "space" or key == "up" or key == "w" then
        player:jump()
    elseif key == "z" or key == "x" or key == "k" then
        player:dash()
    end
end 