Build and optionally run the EV engine.

Usage: /build [SCENE]

1. `make clean && make` — full rebuild, report any errors/warnings
2. If a SCENE argument is provided (e.g. `STATE_SPACE_SUITE`), run: `make dev SCENE=$ARGUMENTS`
3. If no argument, just report build status

Common scenes: STATE_CAR, STATE_LOBBY, STATE_SPACE_LOBBY, STATE_SPACE_CORRIDOR, STATE_SPACE_SUITE, STATE_BALCONY, STATE_PARIS_DREAM
