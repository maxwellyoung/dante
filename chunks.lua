-- chunks.lua
-- Parked experiment: hand-designed segments for a possible future side mode.
-- This file is not part of the primary authored-campaign roadmap.

local Chunks = {}

-- Each chunk has:
-- - id: A unique name.
-- - width, height: Dimensions in tiles.
-- - tags: A table of strings, e.g., {"start", "challenge", "connector", "end"}
-- - difficulty: A number from 1-10.
-- - required_ability: The ability needed to traverse this chunk, e.g., "grapple".
-- - entry_points: A table describing where the player can enter, e.g., {left="middle"}
-- - exit_points: A table describing where the player can exit.
-- - map: A table of strings representing the tile layout.

Chunks.library = {
    -- ===================================================================
    -- Category: Start Chunks (Ability: None)
    -- ===================================================================
    {
        id = "start_simple",
        width = 15,
        height = 7,
        tags = { "start" },
        difficulty = 1,
        required_ability = "none",
        entry_points = {},
        exit_points = { right = "middle" },
        map = {
            "111111111111111",
            "1 P V            ",
            "1 111            ",
            "1                ",
            "1                ",
            "111111111111111",
        },
    },

    -- ===================================================================
    -- Category: Connector Chunks (Ability: None)
    -- ===================================================================
    {
        id = "connector_flat",
        width = 10,
        height = 7,
        tags = { "connector" },
        difficulty = 1,
        required_ability = "none",
        entry_points = { left = "middle" },
        exit_points = { right = "middle" },
        map = {
            "1111111111",
            "1         ",
            "1         ",
            "          ",
            "          ",
            "1         ",
            "1111111111",
        },
    },
    {
        id = "connector_small_hop",
        width = 10,
        height = 7,
        tags = { "connector" },
        difficulty = 1,
        required_ability = "none",
        entry_points = { left = "middle" },
        exit_points = { right = "middle" },
        map = {
            "1111111111",
            "1         ",
            "1         ",
            "       11 ",
            "     111  ",
            "   111    ",
            "1111111111",
        },
    },

    -- ===================================================================
    -- Category: End Chunks (Ability: None)
    -- ===================================================================
    {
        id = "end_simple",
        width = 15,
        height = 7,
        tags = { "end" },
        difficulty = 1,
        required_ability = "none",
        entry_points = { left = "middle" },
        exit_points = {},
        map = {
            "111111111111111",
            "1 C         G  ",
            "1 111       1  ",
            "                 ",
            "                 ",
            "1           1  ",
            "111111111111111",
        },
    },
}

return Chunks
