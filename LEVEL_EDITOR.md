# How to Use the Dante Level Editor

The level editor for _Infernal Ascent_ is built directly into the code. You can create and modify any level by editing the `map_data` tables inside `dante/level.lua`.

## How It Works

Each level is represented by a table of strings. Each character in the string corresponds to a tile in the game world.

```lua
-- Example from level.lua
local map_data = {
    "1111111111111111111111",
    "1 P                G 1",
    "1 111 C E C 11111111 1",
    "1111111111111111111111",
}
```

## Tile Reference

Here is the complete list of tiles you can use to build levels:

| Character   | Object              | Description                                                                     |
| :---------- | :------------------ | :------------------------------------------------------------------------------ |
| `1`         | Wall                | A standard, solid platform.                                                     |
| `2`         | Siren Block         | A pink, pulsating hazard. Kills the player on contact.                          |
| `3`         | Sludge Pool         | A green pool that slows the player and prevents jumping.                        |
| `4`         | Crumbling Block     | A gold block that disappears shortly after being touched.                       |
| ` ` (space) | Air                 | An empty space.                                                                 |
| `P`         | **P**layer Start    | Defines the player's starting position for the level.                           |
| `G`         | **G**ate            | The exit to the next circle.                                                    |
| `C`         | **C**ollectible     | A "Fragment of Memory." The player must collect all of them to unlock the gate. |
| `E`         | **E**nemy           | A patrolling "Lost Soul" enemy.                                                 |
| `V`         | **V**irgil (NPC)    | Your guide. Place him to deliver dialogue.                                      |
| `M`         | **M**oving Platform | The start/end point of a moving platform path.                                  |
| `-`         | Horizontal Path     | Extends a horizontal path for a moving platform.                                |
| `T`         | **T**imed Trap      | A block that appears and disappears on a timer.                                 |

## Creating a New Level

To create a new level (e.g., for `g_current_circle == 5`), simply add a new `elseif` block in `level.lua` and design your map:

```lua
elseif g_current_circle == 5 then
    self.start_pos = { x = 60, y = 112 } -- This will be overridden by 'P'
    self.gate = {x = 720, y = 880, width = TILE_SIZE, height = TILE_SIZE} -- Overridden by 'G'
    self.fragments_required = 3 -- The number of 'C' tiles in your map
    self.npcs = {
        -- You can still define NPC dialogue here
    }
    local map_data = {
        "111111111",
        "1 P      1",
        "1 C 11 C 1",
        "1   E    1",
        "1 C 11 G 1",
        "111111111",
    }
    self:load_map(map_data)
```

### Moving Platforms

To create a horizontal moving platform, place two `M` characters on the same row and connect them with `-` characters. The platform will move back and forth between them.

```lua
local map_data = {
    "111111111111111111111",
    "1                    1",
    "1 M----------------M 1", -- This platform moves horizontally
    "1                    1",
    "111111111111111111111",
}
```

This gives you complete control to build the game you want to play.
