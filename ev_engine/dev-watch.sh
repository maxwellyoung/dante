#!/bin/bash
# ── EV Dev Watch ─────────────────────────────────────────────────
# Auto-rebuild + restart on source changes. Preserves current scene.
#
# Usage:
#   ./dev-watch.sh                    # Watch + restart at last scene
#   ./dev-watch.sh STATE_SPACE_SUITE  # Force a specific scene
# ─────────────────────────────────────────────────────────────────

set -euo pipefail
cd "$(dirname "$0")"

FORCE_SCENE="${1:-}"
STATE_FILE="/tmp/ev_state"
PID_FILE="/tmp/ev_pid"
BIN="./endearing_void"

CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
DIM='\033[0;90m'
RESET='\033[0m'

get_scene() {
    if [ -n "$FORCE_SCENE" ]; then
        echo "$FORCE_SCENE"
    elif [ -f "$STATE_FILE" ]; then
        head -1 "$STATE_FILE" | tr -d '\n'
    else
        echo "STATE_SPACE_LOBBY"
    fi
}

get_hash() {
    cat src/*.c src/*.h 2>/dev/null | md5 2>/dev/null || md5sum src/*.c src/*.h 2>/dev/null | cut -d' ' -f1
}

kill_game() {
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            # Wait briefly for clean exit
            for i in 1 2 3 4 5; do
                kill -0 "$pid" 2>/dev/null || break
                sleep 0.1
            done
            # Force kill if still alive
            kill -0 "$pid" 2>/dev/null && kill -9 "$pid" 2>/dev/null || true
        fi
        rm -f "$PID_FILE"
    fi
}

build_and_run() {
    local scene=$(get_scene)
    printf "${CYAN}[dev-watch]${RESET} Building with SCENE=%s...\n" "$scene"

    kill_game

    # Build dev binary
    if make -j4 -s dev SCENE="$scene" 2>&1 | grep -v "^make\[" | head -20; then
        # make dev already runs the binary, but we want background control
        # So we build separately and run ourselves
        true
    fi

    # Actually: make dev builds AND runs. We need to split that.
    # Build the objects, link, then run in background ourselves.
    make -j4 -s src/main.dev.o src/scene.dev.o src/render.dev.o src/player.dev.o \
         src/audio.dev.o src/lighting.dev.o src/npc.dev.o SCENE="$scene" 2>&1 | head -5 || true

    # Link
    local dev_objs=$(ls src/*.dev.o 2>/dev/null)
    if [ -n "$dev_objs" ]; then
        /usr/bin/clang $dev_objs -o "$BIN" -L/opt/homebrew/lib -lraylib -lm \
            -framework OpenGL -framework Cocoa -framework IOKit \
            -framework CoreAudio -framework CoreVideo 2>&1 || {
            printf "${RED}[dev-watch] Link failed${RESET}\n"
            return 1
        }
    fi

    printf "${GREEN}[dev-watch]${RESET} Launching at %s\n" "$scene"
    $BIN &
    echo $! > "$PID_FILE"
}

cleanup() {
    printf "\n${CYAN}[dev-watch]${RESET} Shutting down...\n"
    kill_game
    exit 0
}
trap cleanup SIGINT SIGTERM

# Initial build
build_and_run

last_hash=$(get_hash)
printf "${CYAN}[dev-watch]${RESET} Watching for changes (Ctrl+C to stop)...\n\n"

while true; do
    sleep 1

    # Check if game crashed
    if [ -f "$PID_FILE" ]; then
        local_pid=$(cat "$PID_FILE")
        if ! kill -0 "$local_pid" 2>/dev/null; then
            printf "${YELLOW}[dev-watch]${RESET} Game exited. Waiting for changes...\n"
            rm -f "$PID_FILE"
        fi
    fi

    hash=$(get_hash)
    if [ "$hash" != "$last_hash" ]; then
        last_hash="$hash"
        printf "\n${CYAN}[dev-watch]${RESET} Change detected: $(date +%H:%M:%S)\n"
        build_and_run || true
    fi
done
