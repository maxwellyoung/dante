local Player = require("player")
local Level = require("level")

function game_reset()
    player = Player:new(400, 450)
    level = Level:new()
end

function love.load()
    -- This function is called exactly once at the beginning of the game.
    print("Infernal Ascent is loading!")
    gameState = "playing"
    game_reset()
end

function love.update(dt)
    -- This function is called every frame. 'dt' is the time since the last frame.
    if gameState == "playing" then
        player:update(dt)
        level:handle_collisions(player)
    end
end

function love.draw()
    -- This function is called every frame to draw things to the screen.
    level:draw()
    player:draw()

    if gameState == "paused" then
        love.graphics.setColor(0, 0, 0, 0.5)
        love.graphics.rectangle("fill", 0, 0, 800, 600)
        love.graphics.setColor(1, 1, 1)
        love.graphics.printf("PAUSED", 0, 280, 800, "center")
    end
end

function love.keypressed(key)
    if key == "escape" then
        if gameState == "playing" then
            gameState = "paused"
        else
            gameState = "playing"
        end
    end

    if key == "r" then
        game_reset()
    end

    if gameState == "playing" then
        if key == "space" or key == "up" or key == "w" then
            player:jump()
        elseif key == "z" or key == "x" or key == "k" then
            player:dash()
        end
    end
end 