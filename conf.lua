function love.conf(t)
    if os.getenv("DANTE_QA_NO_ACTIVATE") == "1" then
        t.window = nil
        return
    end
    t.window.title = "Infernal Ascent"
    t.window.width = 800
    t.window.height = 600
end
