-- generator.lua
-- Parked experiment for a future side mode.
-- The active roadmap is authored rooms and authored circles, not procedural-first generation.

local Chunks = require("chunks")

local Generator = {}
Generator.__index = Generator

function Generator.new()
    return setmetatable({}, Generator)
end

-- A helper function to filter chunks from the library
local function get_chunks_by_tag(tag)
    local filtered = {}
    for _, chunk in ipairs(Chunks.library) do
        for _, t in ipairs(chunk.tags) do
            if t == tag then
                table.insert(filtered, chunk)
            end
        end
    end
    return filtered
end

-- A helper function to stitch two chunks together horizontally
local function stitch_chunks(map_a, map_b)
    local new_map = {}
    local height = math.max(#map_a, #map_b)
    for y = 1, height do
        local row_a = map_a[y] or string.rep(" ", #map_a[1])
        local row_b = map_b[y] or string.rep(" ", #map_b[1])
        new_map[y] = row_a .. row_b
    end
    return new_map
end

function Generator:generate_level(length)
    local _ = self
    local start_chunks = get_chunks_by_tag("start")
    local connector_chunks = get_chunks_by_tag("connector")
    local end_chunks = get_chunks_by_tag("end")

    -- Pick a random start chunk
    local current_level_map = start_chunks[math.random(#start_chunks)].map

    -- Add a number of random connector chunks
    for _ = 1, length do
        local next_chunk = connector_chunks[math.random(#connector_chunks)]
        current_level_map = stitch_chunks(current_level_map, next_chunk.map)
    end

    -- Add a random end chunk
    local end_chunk = end_chunks[math.random(#end_chunks)]
    current_level_map = stitch_chunks(current_level_map, end_chunk.map)
    return current_level_map
end

return Generator
